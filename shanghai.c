#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

// define macro /*{{{*/
#define NN 4
#define ALL 144
//#define BOARD_MAX 59999999
#define BOARD_MAX 60000011
//#define BOARD_MAX 80000023
//#define BOARD_MAX 100000007
#define PYRAMID_LAYER 5
#define PYRAMID_W 12
#define PYRAMID_H 8
#define PRINT_HASH_INFO_SPAN 1000000
/*}}}*/
// struct declaration/*{{{*/
typedef struct HAI_T {
  int i;
  int m;
  int x[3];
  struct HAI_T* l[NN];
  struct HAI_T* r[NN];
  struct HAI_T* u[NN];
  struct HAI_T* d[NN];
} HAI;
typedef struct BOARD_T {
  uint64_t d[3];
  struct BOARD_T* p;
} BOARD;
typedef struct HASH_INFO {
  int used;
  int max_length;
  int max_dc;
  BOARD* max_dc_p;
} HASH_INFO;
/*}}}*/
// static var/*{{{*/
const char name[ALL/4+1][3]  = {/*{{{*/
  "--",
  "1m","2m","3m","4m","5m","6m","7m","8m","9m",
  "1p","2p","3p","4p","5p","6p","7p","8p","9p",
  "1s","2s","3s","4s","5s","6s","7s","8s","9s",
  "jt","jn","js","jp","jw","jb","jr","j4","jm"
};/*}}}*/
/*}}}*/
// prototype/*{{{*/
static int name_index(char* str);
static void init_hai(HAI* h);
static void print_hai(FILE* fp, const HAI* h);
static void print_single_hai(FILE* fp, const HAI hai);
static void init_board(BOARD* b);
static bool can_select(int i, const HAI* h, const BOARD b);
static bool rec(const HAI* hai, BOARD* b, HASH_INFO* hi, BOARD* p);
static void print_move(FILE* fp, BOARD* p, const HAI* hai);
static void read_pyramid(const char* filename, int pyramid[PYRAMID_LAYER][PYRAMID_H][PYRAMID_W]);
static void write_pyramid(const int pyramid[PYRAMID_LAYER][PYRAMID_H][PYRAMID_W], HAI* h);
static bool is_deleted(const int i, const uint64_t d[3]);
static void board_delete(const int ia, const int ib, uint64_t d[3]);
static int hash_value(const uint64_t d[3]);
static int search_board(const BOARD* b, const HASH_INFO* hi, const uint64_t new_d[3]);
static BOARD* register_board(BOARD* b, HASH_INFO* hi, const uint64_t new_d[3], BOARD* p);
static int calc_dc(const uint64_t d[3]);
static void init_hash_info(HASH_INFO* hi, BOARD* board);
static void print_hash_info(FILE* fp, const HASH_INFO* hi);
/*}}}*/

BOARD b[BOARD_MAX];

int main(void) {
  int pyramid[PYRAMID_LAYER][PYRAMID_H][PYRAMID_W];
  const char* datafile= "data.txt";
  read_pyramid(datafile, pyramid);
  fprintf(stderr, "Read datafile: '%s' succeed.\n", datafile);

  fprintf(stderr, "Initialization the tile array: Start\n");
  HAI hai[ALL];
  init_hai(hai);
  fprintf(stderr, "Initialization the tile array: End\n");

  write_pyramid(pyramid, hai);
  print_hai(stderr, hai);

  fprintf(stderr, "Initialization the hash table: Start\n");
  init_board(b);
  HASH_INFO hi;
  init_hash_info(&hi, b);
  fprintf(stderr, "Initialization the hash table: End\n");

  fprintf(stderr, "Calculation of the all possible moves: Start\n");
  const bool is_solved = rec(hai, b, &hi, &(b[0]));
  fprintf(stderr, "Calculation of the all possible moves: End\n");
  fprintf(stderr, "Hash table status:\n");
  print_hash_info(stderr, &hi);

  char *message;
  if (is_solved) message = "Solved. All tiles can be removed. Here is the answer.";
  else message = "Unsolved. Some tiles cannot be removed. Here is the best move.";

  fprintf(stdout, "%s\n", message);
  print_hash_info(stdout, &hi);
  print_move(stdout, hi.max_dc_p, hai);

  return 0;
}

static int name_index(char* str) {/*{{{*/
  int ret;
  for (ret=0; ret<ALL/4+1; ret++) {
    if (name[ret][0] == str[0] && name[ret][1] == str[1]) break;
  }
  return ret;
}/*}}}*/
static void init_hai(HAI* h) {/*{{{*/
  for (int i=0; i<ALL; i++) {
    h[i].i = i;
    h[i].m = -1;
    h[i].x[0] = -1;
    h[i].x[1] = -1;
    h[i].x[2] = -1;

    for (int n=0; n<NN; n++) {
      h[i].l[n] = NULL;
      h[i].r[n] = NULL;
      h[i].u[n] = NULL;
      h[i].d[n] = NULL;
    }
  }
}/*}}}*/
static void print_hai(FILE* fp, const HAI* h) {/*{{{*/
  for (int i=0; i<ALL; i++) {
    fprintf(fp, "%d ", i);
    print_single_hai(fp, h[i]);
    fprintf(fp, "\n");
  }
}/*}}}*/
static void print_single_hai(FILE* fp, const HAI hai) {/*{{{*/
  fprintf(fp, "%s", name[hai.m]);
  fprintf(fp, " [%02d,%02d,%02d]", hai.x[0], hai.x[1], hai.x[2]);

  fprintf(fp, " u:");
  if (hai.u[0] != NULL) fprintf(fp, "%s", name[hai.u[0] -> m]);
  else                   fprintf(fp, "  ");
  for (int nn=1; nn<NN; nn++) if (hai.u[nn] != NULL) fprintf(fp, " %s", name[hai.u[nn] -> m]);

  fprintf(fp, " l:");
  if (hai.l[0] != NULL) fprintf(fp, "%s", name[hai.l[0] -> m]);
  else                   fprintf(fp, "  ");
  for (int nn=1; nn<NN; nn++) if (hai.l[nn] != NULL) fprintf(fp, " %s", name[hai.l[nn] -> m]);

  fprintf(fp, " r:");
  if (hai.r[0] != NULL) fprintf(fp, "%s", name[hai.r[0] -> m]);
  else                   fprintf(fp, "  ");
  for (int nn=1; nn<NN; nn++) if (hai.r[nn] != NULL) fprintf(fp, " %s", name[hai.r[nn] -> m]);

  fprintf(fp, " d:");
  if (hai.d[0] != NULL) fprintf(fp, "%s", name[hai.d[0] -> m]);
  else                   fprintf(fp, "  ");
  for (int nn=1; nn<NN; nn++) if (hai.d[nn] != NULL) fprintf(fp, " %s", name[hai.d[nn] -> m]);
}/*}}}*/
static void init_board(BOARD* b) {/*{{{*/
  for (int j=0; j<BOARD_MAX; j++) {
    b[j].d[0] = UINT64_MAX;
    b[j].d[1] = UINT64_MAX;
    b[j].d[2] = UINT64_MAX;
    b[j].p = NULL;
  }

  b[0].d[0] = 0;
  b[0].d[1] = 0;
  b[0].d[2] = 0;
}/*}}}*/
static bool can_select(int i, const HAI* h, const BOARD b) {/*{{{*/
  const bool nnl_u = (h[i].u[0] != NULL);
  const bool has_u = nnl_u ? (1-is_deleted((h[i].u[0] -> i), b.d)) : false;
  if (has_u) return false;

  const bool nnl_l0 = (h[i].l[0] != NULL);
  const bool nnl_r0 = (h[i].r[0] != NULL);
  const bool nnl_l1 = (h[i].l[1] != NULL);
  const bool nnl_r1 = (h[i].r[1] != NULL);

  const bool has_l0 = nnl_l0 ? (1-is_deleted((h[i].l[0] -> i), b.d)) : false;
  const bool has_r0 = nnl_r0 ? (1-is_deleted((h[i].r[0] -> i), b.d)) : false;
  const bool has_l1 = nnl_l1 ? (1-is_deleted((h[i].l[1] -> i), b.d)) : false;
  const bool has_r1 = nnl_r1 ? (1-is_deleted((h[i].r[1] -> i), b.d)) : false;

  const bool has_l = (has_l0 || has_l1);
  const bool has_r = (has_r0 || has_r1);

  if (has_l && has_r) return false;

  return true;
}/*}}}*/
static bool rec(const HAI* hai, BOARD* b, HASH_INFO* hi, BOARD* p) {/*{{{*/
  if (hi -> used % PRINT_HASH_INFO_SPAN == 0) print_hash_info(stderr, hi);

  if (hi -> used == BOARD_MAX) {
    fprintf(stderr, "ERROR: Memory Size Of The Board Is Not Enough.\n");
    return false;
  }

  if (hi -> max_dc == ALL) {
    fprintf(stderr, "The correct answer is found.\n");
    return true;
  }

  for (int ia=0; ia<ALL; ia++) {
    if (is_deleted(ia, p -> d)) continue;
    if (!can_select(ia, hai, *p)) continue;

    for (int ib=ia+1; ib<ALL; ib++) {
      if (hai[ia].m != hai[ib].m) continue;
      if (is_deleted(ib, p -> d)) continue;
      if (!can_select(ib, hai, *p)) continue;

      uint64_t new_d[3] = {p -> d[0], p -> d[1], p -> d[2]};
      board_delete(ia, ib, new_d);

      if (search_board(b, hi, new_d) >= 0) continue;

      const int old_max_dc = hi -> max_dc;
      BOARD* new_p = register_board(b, hi, new_d, p);
      if (old_max_dc < hi -> max_dc) print_hash_info(stderr, hi);

      if (rec(hai, b, hi, new_p)) return true;
    }
  }
  return false;
}/*}}}*/
static void print_move(FILE* fp, BOARD* p, const HAI* hai) {/*{{{*/
  BOARD* tp = p;
  while (tp != NULL) {
    if (tp -> p == NULL) break;

    for (int i=0; i<ALL; i++) {
      if (is_deleted(i, tp -> d) == is_deleted(i, tp -> p -> d)) continue;
      print_single_hai(fp, hai[i]);
      fprintf(fp, "\n");
    }
    tp = tp -> p;
  }
}/*}}}*/
static void read_pyramid(const char* filename, int pyramid[PYRAMID_LAYER][PYRAMID_H][PYRAMID_W]) {/*{{{*/
  int l=0, h=0, w=0;

  FILE *fp;
  if ((fp = fopen(filename, "r")) == NULL) fprintf(stderr, "File Not Found\n");

  char s[4];
  s[3] = '\0';
  while ((s[0] = getc(fp)) != EOF) {
    if (s[0] == '\n') {
      if ((w == 0) & (h == 0)) continue; // skip empty line
      else fprintf(stderr, "Read File New Layer Error ''' %s '''\n", s);
    }

    s[1] = getc(fp);
    s[2] = getc(fp);
    if (s[2] != ' ' && s[2] != '\n') fprintf(stderr, "Read File Delimitar Error ''' %s '''\n", s);
    if (w == PYRAMID_W-1 && s[2] != '\n') fprintf(stderr, "Read File Line End Error ''' %s '''\n", s);
    s[2] = '\0';

    const int m = name_index(s);
    pyramid[l][h][w] = m;

    w = (w+1) % PYRAMID_W;
    h = ((w == 0) ? h+1 : h) % PYRAMID_H;
    l = ((w == 0) & (h == 0)) ? l+1 : l;
  }

  fclose(fp);
}/*}}}*/
static void write_pyramid(const int pyramid[PYRAMID_LAYER][PYRAMID_H][PYRAMID_W], HAI* hai) {/*{{{*/
  HAI* x_h[PYRAMID_LAYER][PYRAMID_H][PYRAMID_W] = {{{NULL}}};

  int i=0;
  int x[3];
  for (x[0]=0; x[0]<PYRAMID_LAYER; x[0]++) for (x[1]=0; x[1]<PYRAMID_H; x[1]++) for (x[2]=0; x[2]<PYRAMID_W; x[2]++) {
    if (pyramid[x[0]][x[1]][x[2]] == name_index("--")) continue;
    assert(i < ALL);
    hai[i].m = pyramid[x[0]][x[1]][x[2]];
    hai[i].x[0] = x[0];
    hai[i].x[1] = x[1];
    hai[i].x[2] = x[2];
    x_h[x[0]][x[1]][x[2]] = &(hai[i]);
    i++;
  }

  for (x[0]=0; x[0]<PYRAMID_LAYER; x[0]++) for (x[1]=0; x[1]<PYRAMID_H; x[1]++) for (x[2]=0; x[2]<PYRAMID_W; x[2]++) {
    if (x[0] == 0) continue;

    HAI *th = x_h[x[0]][x[1]][x[2]];
    if (th == NULL) continue;

    if (x[0] >= 2) {
      HAI *down = x_h[x[0]-1][x[1]][x[2]];
      if (down != NULL) {
        down -> u[0] = th;
        th -> d[0] = down;
      }
    }

    if (x[2] >= 1) {
      HAI *left = x_h[x[0]][x[1]][x[2]-1];
      if (left != NULL) {
        left -> r[0] = th;
        th -> l[0] = left;
      }
    }

    if (x[2] <= PYRAMID_W-2) {
      HAI *right = x_h[x[0]][x[1]][x[2]+1];
      if (right != NULL) {
        right -> l[0] = th;
        th -> r[0] = right;
      }
    }
  }

  // far left 6p
  hai[0].r[0] = x_h[1][3][0];
  hai[0].r[1] = x_h[1][4][0];
  x_h[1][3][0] -> l[0] = &(hai[0]);
  x_h[1][4][0] -> l[0] = &(hai[0]);

  // far right 5p
  hai[2].l[0] = x_h[1][3][11];
  hai[2].l[1] = x_h[1][4][11];
  x_h[1][3][11] -> r[0] = &(hai[2]);
  x_h[1][4][11] -> r[0] = &(hai[2]);

  // far right 4m
  hai[3].l[0] = &(hai[2]);
  hai[2].r[0] = &(hai[3]);

  // far top 1s
  hai[1].d[0] = x_h[4][3][5];
  hai[1].d[1] = x_h[4][4][5];
  hai[1].d[2] = x_h[4][3][6];
  hai[1].d[3] = x_h[4][4][6];
  x_h[4][3][5] -> u[0] = &(hai[1]);
  x_h[4][4][5] -> u[0] = &(hai[1]);
  x_h[4][3][6] -> u[0] = &(hai[1]);
  x_h[4][4][6] -> u[0] = &(hai[1]);
}/*}}}*/
static bool is_deleted(const int i, const uint64_t d[3]) {/*{{{*/
  uint64_t filt;
  if      (i < 64)  filt = d[0] & ((uint64_t)1 << (i-  0));
  else if (i < 128) filt = d[1] & ((uint64_t)1 << (i- 64));
  else              filt = d[2] & ((uint64_t)1 << (i-128));
  return (filt != 0);
}/*}}}*/
static void board_delete(const int ia, const int ib, uint64_t d[3]) {/*{{{*/
  if      (ia < 64)  d[0] = d[0] | ((uint64_t)1 << (ia-  0));
  else if (ia < 128) d[1] = d[1] | ((uint64_t)1 << (ia- 64));
  else               d[2] = d[2] | ((uint64_t)1 << (ia-128));

  if      (ib < 64)  d[0] = d[0] | ((uint64_t)1 << (ib-  0));
  else if (ib < 128) d[1] = d[1] | ((uint64_t)1 << (ib- 64));
  else               d[2] = d[2] | ((uint64_t)1 << (ib-128));
}/*}}}*/
static int hash_value(const uint64_t d[3]) {/*{{{*/
  return (int)(((d[0] % BOARD_MAX + d[1]) % BOARD_MAX + d[2]) % BOARD_MAX);
}/*}}}*/
static int search_board(const BOARD* b, const HASH_INFO* hi, const uint64_t new_d[3]) {/*{{{*/
  int ret = -1;
  const int hv = hash_value(new_d);
  for (int i=0; i<hi -> max_length; i++) {
    const int j = (hv + i) % BOARD_MAX;
    if (b[j].d[0] == new_d[0] && b[j].d[1] == new_d[1] && b[j].d[2] == new_d[2]) {
      ret = j;
      break;
    }
  }
  return ret;
}/*}}}*/
static BOARD* register_board(BOARD* b, HASH_INFO* hi, const uint64_t new_d[3], BOARD* p) {/*{{{*/
  int new_index = -1;
  const int hv = hash_value(new_d);

  int new_length;
  for (new_length=0; new_length<BOARD_MAX; new_length++) {
    const int j = (hv + new_length) % BOARD_MAX;
    if (b[j].d[0] == UINT64_MAX && b[j].d[1] == UINT64_MAX && b[j].d[2] == UINT64_MAX) {
      new_index = j;
      break;
    }
  }
  assert(new_index > 0);

  b[new_index].d[0] = new_d[0];
  b[new_index].d[1] = new_d[1];
  b[new_index].d[2] = new_d[2];
  b[new_index].p    = p;

  hi -> used = hi -> used + 1;
  if (hi -> max_length < new_length) hi -> max_length = new_length;

  const int dc = calc_dc(new_d);
  if (hi -> max_dc < dc) {
    hi -> max_dc = dc;
    hi -> max_dc_p = &(b[new_index]);
  }

  return &(b[new_index]);
}/*}}}*/
static int calc_dc(const uint64_t d[3]) {/*{{{*/
  int dc = 0;
  for (int i=0; i<ALL; i++) if (is_deleted(i, d)) dc++;
  return dc;
}/*}}}*/
static void init_hash_info(HASH_INFO* hi, BOARD* board) {/*{{{*/
  hi -> used        = 1;
  hi -> max_length  = 1;
  hi -> max_dc      = 0;
  hi -> max_dc_p    = &(board[0]);
}/*}}}*/
static void print_hash_info(FILE* fp, const HASH_INFO* hi) {/*{{{*/
  fprintf(fp, "*");
  fprintf(fp, " rest_tiles:%d/%d", (ALL-hi -> max_dc), ALL);
  fprintf(fp, " hash_table_load_factor:%d/%d", hi -> used, BOARD_MAX);
  fprintf(fp, " max_continuous_hash_length:%d", hi -> max_length);
  fprintf(fp, "\n");
}/*}}}*/
