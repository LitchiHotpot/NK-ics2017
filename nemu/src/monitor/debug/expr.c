#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>

enum {
  TK_NOTYPE = 256,
  /* TODO: Add more token types */
  TK_HEX=257,
  TK_NUM=258,
  TK_REG=259,
  TK_NEG=260,
  TK_POINT=261,
  TK_EQ = 262,
  TK_NOTEQ=263,
  TK_AND=264,
  TK_OR=265
};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {" +", TK_NOTYPE},    // spaces
  {"0x[0-9,a-f]+",TK_HEX},
  {"[1-9][0-9]*|0",TK_NUM},
  {"\\$[a-z]{2,3}",TK_REG},
  {"\\(", '('},
  {"\\)", ')'},
  {"\\*", '*'},
  {"\\/", '/'},
  {"\\+", '+'},         // plus
  {"\\-", '-'},
  {"==", TK_EQ},         // equal
  {"!=", TK_NOTEQ},
  {"&&", TK_AND},
  {"\\|\\|", TK_OR}
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        /*Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);*/
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE:
	    break;
	  default:
	    strncpy(tokens[nr_token].str, substr_start, substr_len);
	    tokens[nr_token].str[substr_len]='\0';
	    tokens[nr_token].type=rules[i].token_type;
	    if(tokens[nr_token].type=='-'&&(nr_token == 0||
	    (tokens[nr_token-1].type!=TK_NUM &&
 	    tokens[nr_token-1].type!=TK_REG &&
 	    tokens[nr_token-1].type!=TK_HEX )))
	    tokens[nr_token].type=TK_NEG;
	    if(tokens[nr_token].type=='*'&&(nr_token == 0||
	    (tokens[nr_token-1].type!=TK_NUM &&
 	    tokens[nr_token-1].type!=TK_REG &&
 	    tokens[nr_token-1].type!=TK_HEX )))
	    tokens[nr_token].type=TK_POINT;
	    nr_token++;
	    break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int p,int q){
  int total=0;
  bool flag=1;
  for(int i=p;i<=q;i++){
    if(tokens[i].type == '(') total++;
    else if(tokens[i].type == ')') total--;
    if(total==0&&i<q) flag=0;  //4+3*(2-1) false
    if(total<0) panic("Parentheses Error!\n");
  }
  if(total!=0) panic("Parentheses Error!\n");
  if(flag) return true;
  return false;
}

int operator_precedence(int op){
    if(op == 262 || op == 263) //TK_EQ,TK_NOTEQ
		return 7;
	else if(op == '+' || op == '-')
		return 4;
	else if(op == '*' || op == '/' || op == '%')
		return 3;
	else if(op == 260 || op ==261) // TK_NEG, TK_POINT
		return 2;
	else if(op == 264) // TK_AND
		return 11;
	else if(op == 265) // TK_OR
		return 12;
	else
		return 0;
}

int dominant_operator(int p,int q){
  int domin=p,left=0;
  bool flag=0;
  for(int i=p;i<=q;i++){
    //if(),do nothing
    if(tokens[i].type=='('){
      left+=1;
      i++;
    while(true) {
        if(tokens[i].type == '(')
          left++;
        else if(tokens[i].type == ')')
          left--;
        i++;
        if(left == 0)
          break;
    }
    if(i > q) break;
    }
        if(tokens[i].type == TK_NUM || tokens[i].type == TK_HEX || tokens[i].type == TK_REG)
          continue;
        if(operator_precedence(tokens[i].type) >= flag) {
            flag = operator_precedence(tokens[i].type);
            domin = i;
            char dest[255] = "\0";
            for(int j = p; j <= q; j++)
                strcat(dest, tokens[j].str);
            //Log("The dominant of %s is tokens[%d] = '%s'", dest, domin, tokens[domin].str);
        }
      }
      return domin;
}

uint32_t eval(int p, int q) {
  if(p > q)
    return -1;

  else if (p == q) {
    // Single token
    if(tokens[p].type == TK_NUM)
      return atoi(tokens[p].str);

    else if(tokens[p].type == TK_HEX){
      int tmp;
      sscanf(tokens[p].str, "%x", &tmp);
      return tmp;
    }
    else if(tokens[p].type == TK_REG) {
      char str[32] = "\0";
      if(strcmp(tokens[p].str, "$EAX") == 0 || strcmp(tokens[p].str, "$eax") == 0){
	sprintf(str, "%8x", cpu.eax);
      }
      else if(strcmp(tokens[p].str, "$EBX") == 0 || strcmp(tokens[p].str, "$ebx") == 0){
	sprintf(str, "%8x", cpu.ebx);
      }
      else if(strcmp(tokens[p].str, "$ECX") == 0 || strcmp(tokens[p].str, "$ecx") == 0){
	sprintf(str, "%8x", cpu.ecx);
      }
      else if(strcmp(tokens[p].str, "$EDX") == 0 || strcmp(tokens[p].str, "$edx") == 0){
	sprintf(str, "%8x", cpu.edx);
      }
      else if(strcmp(tokens[p].str, "$EBP") == 0 || strcmp(tokens[p].str, "$ebp") == 0){
	sprintf(str, "%8x", cpu.ebp);
      }
      else if(strcmp(tokens[p].str, "$ESP") == 0 || strcmp(tokens[p].str, "$esp") == 0){
	sprintf(str, "%8x", cpu.esp);
      }
      else if(strcmp(tokens[p].str, "$ESI") == 0 || strcmp(tokens[p].str, "$esi") == 0){
	sprintf(str, "%8x", cpu.esi);
      }
      else if(strcmp(tokens[p].str, "$EDI") == 0 || strcmp(tokens[p].str, "$edi") == 0){
	sprintf(str, "%8x", cpu.edi);
      }
      else if(strcmp(tokens[p].str, "$AX") == 0 || strcmp(tokens[p].str, "$ax") == 0){
	sprintf(str, "%8x", cpu.eax & 0xffff);
      }
      else if(strcmp(tokens[p].str, "$BX") == 0 || strcmp(tokens[p].str, "$bx") == 0){
	sprintf(str, "%8x", cpu.ebx & 0xffff);
      }
      else if(strcmp(tokens[p].str, "$CX") == 0 || strcmp(tokens[p].str, "$cx") == 0){
	sprintf(str, "%8x", cpu.ecx & 0xffff);
      }
      else if(strcmp(tokens[p].str, "$DX") == 0 || strcmp(tokens[p].str, "$dx") == 0){
	sprintf(str, "%8x", cpu.edx & 0xffff);
      }
      else if(strcmp(tokens[p].str, "$BP") == 0 || strcmp(tokens[p].str, "$bp") == 0){
	sprintf(str, "%8x", cpu.ebp & 0xffff);
      }
      else if(strcmp(tokens[p].str, "$SP") == 0 || strcmp(tokens[p].str, "$sp") == 0){
	sprintf(str, "%8x", cpu.esp & 0xffff);
      }
      else if(strcmp(tokens[p].str, "$SI") == 0 || strcmp(tokens[p].str, "$si") == 0){
	sprintf(str, "%8x", cpu.esi & 0xffff);
      }
      else if(strcmp(tokens[p].str, "$DI") == 0 || strcmp(tokens[p].str, "$di") == 0){
	sprintf(str, "%8x", cpu.edi & 0xffff);
      }
      else if(strcmp(tokens[p].str, "$AH") == 0 || strcmp(tokens[p].str, "$ah") == 0){
	sprintf(str, "%8x", (cpu.eax >> 8) & 0xff);
      }
      else if(strcmp(tokens[p].str, "$AL") == 0 || strcmp(tokens[p].str, "$al") == 0){
	sprintf(str, "%8x", cpu.eax & 0xff);
      }
      else if(strcmp(tokens[p].str, "$BH") == 0 || strcmp(tokens[p].str, "$bh") == 0){
	sprintf(str, "%8x", (cpu.ebx >> 8) & 0xff);
      }
      else if(strcmp(tokens[p].str, "$BL") == 0 || strcmp(tokens[p].str, "$bl") == 0){
	sprintf(str, "%8x", cpu.ebx & 0xff);
      }
      else if(strcmp(tokens[p].str, "$CH") == 0 || strcmp(tokens[p].str, "$ch") == 0){
	sprintf(str, "%8x", (cpu.ecx >> 8) & 0xff);
      }
      else if(strcmp(tokens[p].str, "$CL") == 0 || strcmp(tokens[p].str, "$cl") == 0){
	sprintf(str, "%8x", cpu.ecx & 0xff);
      }
      else if(strcmp(tokens[p].str, "$DH") == 0 || strcmp(tokens[p].str, "$dh") == 0){
	sprintf(str, "%8x", (cpu.edx >> 8) & 0xff);
      }
      else if(strcmp(tokens[p].str, "$DL") == 0 || strcmp(tokens[p].str, "$dl") == 0){
	sprintf(str, "%8x", cpu.edx & 0xff);
      }
      else if(strcmp(tokens[p].str, "$eip") == 0){
                sprintf(str, "%8x", cpu.eip);
      }
      uint32_t tmp;
      sscanf(str, "%x", &tmp);
      return tmp;
    }
    panic("Error: wrong tokens[%d]!\n", p);
  }
    else if(check_parentheses(p, q) == true)
        return eval(p + 1, q - 1);
    else {
        int op = dominant_operator(p, q);
        uint32_t val1 = eval(p, op - 1);
        uint32_t val2 = eval(op + 1, q);
        switch(tokens[op].type) {
            case '+': return val1 + val2;
            case '-': return val1 - val2;
            case '*': return val1 * val2;
            case '/':
            if(val2 == 0)
                panic("Error: divide zero!\n");
            else
                return val1 / val2;
            case '%':
                if(val2 == 2)
                panic("Error: mode zero!\n");
            else
                return val1 % val2;
          case 262: return (val1 == val2);
          case 263: return (val1 != val2);
          case 260: return -val2;
          case 259:
          {
            vaddr_t addr = val2;
            return vaddr_read(addr, 1);
          }
          case 264: return (val1 && val2);
          case 265: return (val1 || val2);
          default: panic("Error: tokens[%d]=%s, val1=%d, val2=%d\n", op, tokens[op].str, val1, val2);
        }
      }
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  uint32_t result = eval(0,nr_token-1);

  return result;
}

