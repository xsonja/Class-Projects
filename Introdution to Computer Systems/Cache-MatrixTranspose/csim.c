#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "cachelab.h"

int tag_bit, set_bit, block_bit, E;
int misses = 0;
int hits = 0;
int evictions = 0;

typedef struct {
  unsigned long long int tbit;
  unsigned long long int sbit;
  unsigned long long int bbit;
}address;
  
typedef struct {
  int last_used;
  int vbit;
  address *add;
}line;
 	
typedef struct {
  line **l;
}set;
 	
typedef struct {
  set **s;
}cache;

int mask(unsigned long long int x, int y)
{
  return x & ((1 << y) - 1);
}

cache *init_cache()
{
  int s_ct = 0;
  int l_ct = 0;
  int S = 1 << set_bit;
  cache *cache_new = malloc(sizeof(cache));
  for( ; s_ct < S; s_ct++){
    cache_new->s[s_ct] = malloc(sizeof(set));
    for( ; l_ct < E; l_ct++){
      cache_new->s[s_ct]->l = malloc(sizeof(line));
      cache_new->s[s_ct]->l[l_ct]->last_used = 0;
      cache_new->s[s_ct]->l[l_ct]->vbit = 0;
      cache_new->s[s_ct]->l[l_ct]->add = malloc(sizeof(address));
    }
  }
  return cache_new;
}

address *make_address(unsigned long long int add)
{
  //int t = 64 - set_bit - block_bit;
  address *new_add = malloc(sizeof(address));
  new_add->bbit = mask(add,block_bit);
  add>>=block_bit;
  new_add->sbit = mask(add, set_bit);
  add>>=set_bit;
  new_add->tbit = add;
  return new_add;
}

void line_free(line *l)
{
  free(l->add);
  free(l);
  return;
}

void set_free(set *s)
{
  int i;
  for(i = 0; i < E; i++){
    line_free(s->l[i]);
  }
  free(s);
  return;
}

void cache_free(cache *c)
{
  int S = (1 << set_bit);
  int i;
  for(i = 0; i < S; i++){
    set_free(c->s[i]);
  }
  free(c->s);
  free(c);
  return;
}

int find_free_line(set *s)
{
  int i = 0;
  //int res = 0;
  for(; i < E; i++){
    if(s->l[i]->vbit == 0){
      return i;
    }
  }
  return -1;
}

int find_lru(set *s)
/*to know which line to evict in the case where there are no 
free lines*/
{
  int i = 0;
  int lru = -10000000;
  int temp;
  for( ; i < E; i++){
    temp = s->l[i]->last_used;
    if(temp > lru)
      lru = i;
  }
  return lru;
}

void update_lru(set *s)
{
  int i;
  for(i = 0; i < E; i++){
    s->l[i]->last_used++;
  }
  return;
}

void update_line(cache *c, address *a, int lind)
/*to either evict a line or to add a new address into the line*/
{
  c->s[a->sbit]->l[lind]->last_used = 0;
  c->s[a->sbit]->l[lind]->vbit = 1;
  c->s[a->sbit]->l[lind]->add = a;
  return;
}

int is_address_there(cache *c, address *a)
{
  int i = 0;
  line *l; 
  for(; i < E; i++){
    l = c->s[a->sbit]->l[i];
    if(l->vbit == 1){
      if(l->add->tbit == a-> tbit){
	return 1;
      }
    }
  }
  return 0;
}

void sim_helper(cache *c, address *a)
{
  int line = find_free_line(c->s[a->sbit]);
  if(is_address_there(c, a) == 0){
    misses++;
    if(line < 0){
      evictions++;
      line = find_lru(c->s[a->sbit]);
      update_lru(c->s[a->sbit]);
      update_line(c, a, line);
    }
    else if(line>=0) {
      update_lru(c->s[a->sbit]);
      update_line(c, a, line);
    }
    else {
      fprintf(stderr, "What did you do?");
      exit(1);
    }
  }
  else
    hits++;
  return; 
}

void simulate(cache *c, FILE *trace)
{
  char *str;
  address* ad;
  long long int address_hex;
  while(fgets(str, 10000, trace) != NULL){
    if(str[0] != 'I'){
      sscanf(str+=3, "%llx", &address_hex);
      ad = make_address((unsigned)address_hex);
      switch(str[1]) {
      case 'S':
	sim_helper(c, ad);
	break;
      case 'L':
	sim_helper(c, ad);
	break;
      case 'M':
	sim_helper(c, ad);
	sim_helper(c, ad);
	break;
      default :
	break;
      }
    }
  }
  return;	
}

int main(int argc, char *argv[])
{
  FILE *t;
  //char n;
  cache *c;
  int cntr = 0;
  if (argc == 0) {
	  fprintf(stderr, "INPUT ARGS PLEASE");
	  exit(1);
  } else {
	  for (cntr = 1; cntr < argc; cntr++) {
			if (strcmp(argv[cntr], "-s") == 0) {
				set_bit = atoi(argv[cntr+=1]);
			}
			else if (strcmp(argv[cntr], "-b") == 0) {
				block_bit = atoi(argv[cntr+=1]);
			}
			else if (strcmp(argv[cntr], "-E") == 0) {
				E = atoi(argv[cntr+=1]);
			}
			else if (strcmp(argv[cntr], "-t") == 0) {
				t = fopen(argv[cntr+=1], "r");
			}
	  }
  }
<<<<<<< .mine
 
=======
>>>>>>> .r33
    c = init_cache();
    simulate(c, t);
    
    printSummary(hits, misses, evictions);
    cache_free(c);
    return 0;
}
