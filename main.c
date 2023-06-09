#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define m_set_bit(word, index) ((word) |= (1ULL << (U64)(index)))
#define m_get_bit(word, index) ((word) & (1ULL << (U64)(index)))
#define m_pop_bit(word, index) ((word) &= ~(1ULL << (U64)(index)))
#define m_zero_least(word) ((word) &= (word - 1ULL))
#define m_encode_move(source, dest) (U16)(source) | (U16)((dest) << 6U)
#define m_decode_move(source, dest, move) source = (int)((move) & 0x3fU); dest = (int)(((move) >> 6U) & 0x3fU)
#define U64 unsigned long long
#define U16 unsigned short

typedef struct{
    U64 bits[24];
    int side;
} Position;

typedef struct{
    U16 moves[100];
    int count;
} Move_tab;

typedef struct{
    int score[4];
} Score;

enum Player
{
    Player1,
    Player2,
    Player3,
    Player4,
};

enum Player_type
{
    Human,
    Computer,
};

enum Square
{
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1,
};

enum Piece
{
    P1,
    N1,
    B1,
    R1,
    Q1,
    K1,
    P2,
    N2,
    B2,
    R2,
    Q2,
    K2,
    P3,
    N3,
    B3,
    R3,
    Q3,
    K3,
    P4,
    N4,
    B4,
    R4,
    Q4,
    K4,
};

int min(int a, int b);
int max(int a, int b);
static inline U64 get_ally_bitposition(Position* position, int color);
static inline U64 get_all_bitposition(Position* position);
static inline U64 get_enemy_bitposition(Position* position, int color);
static inline int get_population_count(U64 bitposition);
static inline int get_least_bit_index(U64 bitposition);
void print_bitposition(U64 word);
void print_score(Score score);
void print_position(Position* pos);
static inline U64 generate_bishop_attacks(int square, U64 blocker);
static inline U64 generate_rook_attacks(int square, U64 blocker);
void set_standard_position(Position* pos);
Move_tab move_generator(Position* position);
static inline Score evaluate_position_simple(Position* position);
static inline int evaluate_position_simple_for_player(Position* position, enum Player ally);
void make_move(Position* position, U16 move);
int paranoid_alpha_beta(Position* position, enum Player player, int alpha, int beta, int root, int ply, U16* best_mov, int* nodes);
void query_engine_for_move(Position* position);
void query_player_for_move(Position* position);
void read_move(U16 word);
U16 parse_move();
int end_game_if_finished(Position* position);
int main();


const char *colors[] = {
    "\x1B[0m",
    "\x1B[31m",
    "\x1B[32m",
    "\x1B[33m",
    "\x1B[34m",
};

const char *index_to_coord[] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
};

char* unicode_pieces[6] = {"♟︎ ", "♞ ", "♝ ", "♜ ", "♛ ", "♚ "};
char* unicode_numbers[8] = {"１","２","３","４","５","６","７","８"};
char* unicode_letters[8] = {"Ａ","Ｂ","Ｃ","Ｄ","Ｅ","Ｆ","Ｇ","Ｈ"};

//evals
const int piece_val[24] = 
{
    1, 3, 3, 5, 9, 200, 1, 3, 3, 5, 9, 200, 1, 3, 3, 5, 9, 200, 1, 3, 3, 5, 9, 200,
};
//attack tables
const U64 queening_masks[4] = {
    0x80808080808080ff,
    0x1010101010101ff,
    0xff01010101010101,
    0xff80808080808080,
};
const U64 pawn_moves[64] =
{
    0x102, 0x205, 0x40a, 0x814, 
    0x1028, 0x2050, 0x40a0, 0x8040, 
    0x10201, 0x20502, 0x40a04, 0x81408, 
    0x102810, 0x205020, 0x40a040, 0x804080, 
    0x1020100, 0x2050200, 0x40a0400, 0x8140800, 
    0x10281000, 0x20502000, 0x40a04000, 0x80408000, 
    0x102010000, 0x205020000, 0x40a040000, 0x814080000, 
    0x1028100000, 0x2050200000, 0x40a0400000, 0x8040800000, 
    0x10201000000, 0x20502000000, 0x40a04000000, 0x81408000000, 
    0x102810000000, 0x205020000000, 0x40a040000000, 0x804080000000, 
    0x1020100000000, 0x2050200000000, 0x40a0400000000, 0x8140800000000, 
    0x10281000000000, 0x20502000000000, 0x40a04000000000, 0x80408000000000, 
    0x102010000000000, 0x205020000000000, 0x40a040000000000, 0x814080000000000, 
    0x1028100000000000, 0x2050200000000000, 0x40a0400000000000, 0x8040800000000000, 
    0x201000000000000, 0x502000000000000, 0xa04000000000000, 0x1408000000000000, 
    0x2810000000000000, 0x5020000000000000, 0xa040000000000000, 0x4080000000000000,
};
const U64 pawn_attacks[64] =
{
    0x200, 0x500, 0xa00, 0x1400, 
    0x2800, 0x5000, 0xa000, 0x4000, 
    0x20002, 0x50005, 0xa000a, 0x140014, 
    0x280028, 0x500050, 0xa000a0, 0x400040, 
    0x2000200, 0x5000500, 0xa000a00, 0x14001400, 
    0x28002800, 0x50005000, 0xa000a000, 0x40004000, 
    0x200020000, 0x500050000, 0xa000a0000, 0x1400140000, 
    0x2800280000, 0x5000500000, 0xa000a00000, 0x4000400000, 
    0x20002000000, 0x50005000000, 0xa000a000000, 0x140014000000, 
    0x280028000000, 0x500050000000, 0xa000a0000000, 0x400040000000, 
    0x2000200000000, 0x5000500000000, 0xa000a00000000, 0x14001400000000, 
    0x28002800000000, 0x50005000000000, 0xa000a000000000, 0x40004000000000, 
    0x200020000000000, 0x500050000000000, 0xa000a0000000000, 0x1400140000000000, 
    0x2800280000000000, 0x5000500000000000, 0xa000a00000000000, 0x4000400000000000, 
    0x2000000000000, 0x5000000000000, 0xa000000000000, 0x14000000000000, 
    0x28000000000000, 0x50000000000000, 0xa0000000000000, 0x40000000000000,
};
const U64 knight_attacks[64] = 
{
    0x20400, 0x50800, 0xa1100, 0x142200, 
    0x284400, 0x508800, 0xa01000, 0x402000, 
    0x2040004, 0x5080008, 0xa110011, 0x14220022, 
    0x28440044, 0x50880088, 0xa0100010, 0x40200020, 
    0x204000402, 0x508000805, 0xa1100110a, 0x1422002214, 
    0x2844004428, 0x5088008850, 0xa0100010a0, 0x4020002040, 
    0x20400040200, 0x50800080500, 0xa1100110a00, 0x142200221400, 
    0x284400442800, 0x508800885000, 0xa0100010a000, 0x402000204000, 
    0x2040004020000, 0x5080008050000, 0xa1100110a0000, 0x14220022140000, 
    0x28440044280000, 0x50880088500000, 0xa0100010a00000, 0x40200020400000, 
    0x204000402000000, 0x508000805000000, 0xa1100110a000000, 0x1422002214000000, 
    0x2844004428000000, 0x5088008850000000, 0xa0100010a0000000, 0x4020002040000000, 
    0x400040200000000, 0x800080500000000, 0x1100110a00000000, 0x2200221400000000, 
    0x4400442800000000, 0x8800885000000000, 0x100010a000000000, 0x2000204000000000, 
    0x4020000000000, 0x8050000000000, 0x110a0000000000, 0x22140000000000, 
    0x44280000000000, 0x88500000000000, 0x10a00000000000, 0x20400000000000,
};
const U64 king_attacks[64] = 
{
    0x302, 0x705, 0xe0a, 0x1c14, 
    0x3828, 0x7050, 0xe0a0, 0xc040, 
    0x30203, 0x70507, 0xe0a0e, 0x1c141c, 
    0x382838, 0x705070, 0xe0a0e0, 0xc040c0, 
    0x3020300, 0x7050700, 0xe0a0e00, 0x1c141c00, 
    0x38283800, 0x70507000, 0xe0a0e000, 0xc040c000, 
    0x302030000, 0x705070000, 0xe0a0e0000, 0x1c141c0000, 
    0x3828380000, 0x7050700000, 0xe0a0e00000, 0xc040c00000, 
    0x30203000000, 0x70507000000, 0xe0a0e000000, 0x1c141c000000, 
    0x382838000000, 0x705070000000, 0xe0a0e0000000, 0xc040c0000000, 
    0x3020300000000, 0x7050700000000, 0xe0a0e00000000, 0x1c141c00000000, 
    0x38283800000000, 0x70507000000000, 0xe0a0e000000000, 0xc040c000000000, 
    0x302030000000000, 0x705070000000000, 0xe0a0e0000000000, 0x1c141c0000000000, 
    0x3828380000000000, 0x7050700000000000, 0xe0a0e00000000000, 0xc040c00000000000, 
    0x203000000000000, 0x507000000000000, 0xa0e000000000000, 0x141c000000000000, 
    0x2838000000000000, 0x5070000000000000, 0xa0e0000000000000, 0x40c0000000000000,
};
const U64 rook_possible_attacks[64] = 
{
    0x1010101010101fe, 0x2020202020202fd, 0x4040404040404fb, 0x8080808080808f7,
    0x10101010101010ef, 0x20202020202020df, 0x40404040404040bf, 0x808080808080807f, 
    0x10101010101fe01, 0x20202020202fd02, 0x40404040404fb04, 0x80808080808f708,
    0x101010101010ef10, 0x202020202020df20, 0x404040404040bf40, 0x8080808080807f80, 
    0x101010101fe0101, 0x202020202fd0202, 0x404040404fb0404, 0x808080808f70808,
    0x1010101010ef1010, 0x2020202020df2020, 0x4040404040bf4040, 0x80808080807f8080, 
    0x1010101fe010101, 0x2020202fd020202, 0x4040404fb040404, 0x8080808f7080808,
    0x10101010ef101010, 0x20202020df202020, 0x40404040bf404040, 0x808080807f808080, 
    0x10101fe01010101, 0x20202fd02020202, 0x40404fb04040404, 0x80808f708080808,
    0x101010ef10101010, 0x202020df20202020, 0x404040bf40404040, 0x8080807f80808080, 
    0x101fe0101010101, 0x202fd0202020202, 0x404fb0404040404, 0x808f70808080808,
    0x1010ef1010101010, 0x2020df2020202020, 0x4040bf4040404040, 0x80807f8080808080, 
    0x1fe010101010101, 0x2fd020202020202, 0x4fb040404040404, 0x8f7080808080808,
    0x10ef101010101010, 0x20df202020202020, 0x40bf404040404040, 0x807f808080808080, 
    0xfe01010101010101, 0xfd02020202020202, 0xfb04040404040404, 0xf708080808080808,
    0xef10101010101010, 0xdf20202020202020, 0xbf40404040404040, 0x7f80808080808080,
};
const U64 bishop_possible_attacks[64] = 
{
    0x8040201008040200, 0x80402010080500, 0x804020110a00, 0x8041221400,
    0x182442800, 0x10204885000, 0x102040810a000, 0x102040810204000, 
    0x4020100804020002, 0x8040201008050005, 0x804020110a000a, 0x804122140014,
    0x18244280028, 0x1020488500050, 0x102040810a000a0, 0x204081020400040, 
    0x2010080402000204, 0x4020100805000508, 0x804020110a000a11, 0x80412214001422,
    0x1824428002844, 0x102048850005088, 0x2040810a000a010, 0x408102040004020, 
    0x1008040200020408, 0x2010080500050810, 0x4020110a000a1120, 0x8041221400142241,
    0x182442800284482, 0x204885000508804, 0x40810a000a01008, 0x810204000402010, 
    0x804020002040810, 0x1008050005081020, 0x20110a000a112040, 0x4122140014224180,
    0x8244280028448201, 0x488500050880402, 0x810a000a0100804, 0x1020400040201008, 
    0x402000204081020, 0x805000508102040, 0x110a000a11204080, 0x2214001422418000,
    0x4428002844820100, 0x8850005088040201, 0x10a000a010080402, 0x2040004020100804, 
    0x200020408102040, 0x500050810204080, 0xa000a1120408000, 0x1400142241800000,
    0x2800284482010000, 0x5000508804020100, 0xa000a01008040201, 0x4000402010080402, 
    0x2040810204080, 0x5081020408000, 0xa112040800000, 0x14224180000000,
    0x28448201000000, 0x50880402010000, 0xa0100804020100, 0x40201008040201, 
};
int min(int a, int b){
    return a < b ? a : b;
}
int max(int a, int b){
    return a < b ? b : a;
}
static inline U64 get_ally_bitposition(Position* position, int color){
    U64 temp = 0ULL;
    for(int i = 0; i < 6; i++) temp |= position->bits[i + color * 6];
    return temp;
}
static inline U64 get_all_bitposition(Position* position){
    U64 temp = 0ULL;
    for(int i = 0; i < 24; i++) temp |= position->bits[i];
    return temp;
}
static inline U64 get_enemy_bitposition(Position* position, int color){
    U64 temp = get_all_bitposition(position);
    return temp - get_ally_bitposition(position, color);
}
static inline int get_population_count(U64 bitposition)
{
    int count = 0;
    while (bitposition){
        count++;
        bitposition &= bitposition - 1;
    }
    return count;
}

static inline int get_least_bit_index(U64 bitposition)
{
    if (bitposition) return get_population_count((bitposition & -bitposition) - 1);
    return -1;
}

void print_bitposition(U64 word){
    for(int i = 0; i < 8; i++){
        printf("\n\t");
        for(int j = 0; j < 8; j++){
            printf("%s", m_get_bit(word, i * 8 + j) ? "x " : ". ");
        }
    }
    printf("\n0x%llx\n", word);
}
void print_score(Score score){
    printf("\n");
    for(int i = 0; i < 4; i++){
        printf("%s%s's score: %d%s\n",colors[i + 1], unicode_pieces[5], score.score[i], colors[0]);
    }
}
void print_position(Position* pos){
    for (int i = 0; i < 8; i++)
    {
        printf("\n%s", unicode_numbers[7 - i]);
        for (int j = 0; j < 8; j++)
        {
            if(m_get_bit(get_all_bitposition(pos), i*8 + j)){
                for(int piece = 0; piece < 24; piece++)
                {
                    printf("%s%s%s",colors[piece / 6 + 1],  m_get_bit(pos->bits[piece], i*8+j) ? unicode_pieces[piece % 6] : "", colors[0]);
                }
            }
            else printf("  ");
        }
    }
    printf("\n  ");
    for (int i = 0; i < 8; i++)
    {
        printf("%s", unicode_letters[i]);
    }
    printf("\nSide to move: %s%s%s\n",colors[pos->side % 4 + 1], unicode_pieces[5], colors[0]);
}
static inline U64 generate_bishop_attacks(int square, U64 blocker){
    U64 temp = 0;
    int i = square / 8;
    int j = square & 0x7;
    int rank = i;
    int file = j;
    while(rank < 7 && file < 7){
        m_set_bit(temp, (++rank)*8+(++file));
        if(m_get_bit(blocker, rank*8+file)) break;
    }
    rank = i;
    file = j;
    while(rank > 0 && file < 7){
        m_set_bit(temp, (--rank)*8+(++file));
        if(m_get_bit(blocker, rank*8+file)) break;
    } 
    rank = i;
    file = j;
    while(rank > 0 && file > 0){
        m_set_bit(temp, (--rank)*8+(--file));
        if(m_get_bit(blocker, rank*8+file)) break;
    }
    rank = i;
    file = j;
    while(rank < 7 && file > 0){
        m_set_bit(temp, (++rank)*8+(--file));
        if(m_get_bit(blocker, rank*8+file)) break;
    } 
    return temp;
}
static inline U64 generate_rook_attacks(int square, U64 blocker){
    U64 temp = 0;
    int i = square / 8;
    int j = square & 0x7;
    int rank = i;
    int file = j;
    while(rank < 7){
        m_set_bit(temp, (++rank)*8+(file));
        if(m_get_bit(blocker, rank*8+file)) break;
    }
    rank = i;
    file = j;
    while(rank > 0){
        m_set_bit(temp, (--rank)*8+(file));
        if(m_get_bit(blocker, rank*8+file)) break;
    } 
    rank = i;
    file = j;
    while(file > 0){
        m_set_bit(temp, (rank)*8+(--file));
        if(m_get_bit(blocker, rank*8+file)) break;
    }
    rank = i;
    file = j;
    while(file < 7){
        m_set_bit(temp, (rank)*8+(++file));
        if(m_get_bit(blocker, rank*8+file)) break;
    } 
    return temp;
}
void set_standard_position(Position* pos){
    pos->bits[P1] = 0x0404070000000000ULL;
    pos->bits[K1] = 0x0100000000000000ULL;
    pos->bits[Q1] = 0x0000000000000000ULL;
    pos->bits[R1] = 0x0001000000000000ULL;
    pos->bits[B1] = 0x0200000000000000ULL;
    pos->bits[N1] = 0x0002000000000000ULL;
    pos->bits[P2] = 0x2020e00000000000ULL;
    pos->bits[K2] = 0x8000000000000000ULL;
    pos->bits[Q2] = 0x0000000000000000ULL;
    pos->bits[R2] = 0x4000000000000000ULL;
    pos->bits[B2] = 0x0080000000000000ULL;
    pos->bits[N2] = 0x0040000000000000ULL;
    pos->bits[P3] = 0x0000000000e02020ULL;
    pos->bits[K3] = 0x0000000000000080ULL;
    pos->bits[Q3] = 0x0000000000000000ULL;
    pos->bits[R3] = 0x0000000000008000ULL;
    pos->bits[B3] = 0x0000000000000040ULL;
    pos->bits[N3] = 0x0000000000004000ULL;
    pos->bits[P4] = 0x0000000000070404ULL;
    pos->bits[K4] = 0x0000000000000001ULL;
    pos->bits[Q4] = 0x0000000000000000ULL;
    pos->bits[R4] = 0x0000000000000002ULL;
    pos->bits[B4] = 0x0000000000000100ULL;
    pos->bits[N4] = 0x0000000000000200ULL;
    pos->side = 0;
}
Move_tab move_generator(Position* position){
    Move_tab tab;
    tab.count = 0;
    enum Player ally = position->side % 4;
    U64 dest_candidate;
    int candidate_count, source, dest;

    U64 knights = position->bits[ally * 6 + N1];
    int knight_count = get_population_count(knights);
    for(int i = 0; i < knight_count; i++){
        source = get_least_bit_index(knights);
        m_zero_least(knights);
        dest_candidate = knight_attacks[source];
        candidate_count = get_population_count(dest_candidate);
        for(int j = 0; j < candidate_count; j++){
            dest = get_least_bit_index(dest_candidate);
            m_zero_least(dest_candidate);
            if(m_get_bit(get_ally_bitposition(position, ally), dest) == 0ULL){
                tab.moves[tab.count++] = m_encode_move(source, dest);
            }
        }
    }
    
    U64 pawns = position->bits[ally * 6 + P1];
    int pawn_count = get_population_count(pawns);
    for(int i = 0; i < pawn_count; i++){
        source = get_least_bit_index(pawns);
        m_zero_least(pawns);
        
        dest_candidate = pawn_attacks[source];
        candidate_count = get_population_count(dest_candidate);
        for(int j = 0; j < candidate_count; j++){
            dest = get_least_bit_index(dest_candidate);
            m_zero_least(dest_candidate);
            if(m_get_bit(get_enemy_bitposition(position, ally), dest)){
                tab.moves[tab.count++] = m_encode_move(source, dest);
            }
        }
        
        dest_candidate = pawn_moves[source];
        candidate_count = get_population_count(dest_candidate);
        for(int j = 0; j < candidate_count; j++){
            dest = get_least_bit_index(dest_candidate);
            m_zero_least(dest_candidate);
            if(!m_get_bit(get_all_bitposition(position), dest)){
                tab.moves[tab.count++] = m_encode_move(source, dest);
            }
        }
    }
    
    U64 kings = position->bits[ally * 6 + K1];
    int king_count = get_population_count(kings);
    for(int i = 0; i < king_count; i++){
        source = get_least_bit_index(kings);
        m_zero_least(kings);
        dest_candidate = king_attacks[source];
        candidate_count = get_population_count(dest_candidate);
        for(int j = 0; j < candidate_count; j++){
            dest = get_least_bit_index(dest_candidate);
            m_zero_least(dest_candidate);
            if(m_get_bit(get_ally_bitposition(position, ally), dest) == 0ULL){
                tab.moves[tab.count++] = m_encode_move(source, dest);
            }
        }
    }
    
    U64 bishops = position->bits[ally * 6 + B1];
    int bishop_count = get_population_count(bishops);
    for(int i = 0; i < bishop_count; i++){
        source = get_least_bit_index(bishops);
        m_zero_least(bishops);
        dest_candidate = generate_bishop_attacks(source, get_all_bitposition(position));
        candidate_count = get_population_count(dest_candidate);
        for(int j = 0; j < candidate_count; j++){
            dest = get_least_bit_index(dest_candidate);
            m_zero_least(dest_candidate);
            if(m_get_bit(get_ally_bitposition(position, ally), dest) == 0ULL){
                tab.moves[tab.count++] = m_encode_move(source, dest);
            }
        }
    }

    U64 rooks = position->bits[ally * 6 + R1];
    int rook_count = get_population_count(rooks);
    for(int i = 0; i < rook_count; i++){
        source = get_least_bit_index(rooks);
        m_zero_least(rooks);
        dest_candidate = generate_rook_attacks(source, get_all_bitposition(position));
        candidate_count = get_population_count(dest_candidate);
        for(int j = 0; j < candidate_count; j++){
            dest = get_least_bit_index(dest_candidate);
            m_zero_least(dest_candidate);
            if(m_get_bit(get_ally_bitposition(position, ally), dest) == 0ULL){
                tab.moves[tab.count++] = m_encode_move(source, dest);
            }
        }
    }

    U64 queens = position->bits[ally * 6 + Q1];
    int queen_count = get_population_count(queens);
    for(int i = 0; i < queen_count; i++){
        source = get_least_bit_index(queens);
        m_zero_least(queens);
        dest_candidate = generate_rook_attacks(source, get_all_bitposition(position)) | generate_bishop_attacks(source, get_all_bitposition(position));
        candidate_count = get_population_count(dest_candidate);
        for(int j = 0; j < candidate_count; j++){
            dest = get_least_bit_index(dest_candidate);
            m_zero_least(dest_candidate);
            if(m_get_bit(get_ally_bitposition(position, ally), dest) == 0ULL){
                tab.moves[tab.count++] = m_encode_move(source, dest);
            }
        }
    }
    return tab;
}
static inline Score evaluate_position_simple(Position* position){
    Score out;
    for(int i = 0; i < 4; i++){
        out.score[i] = 0;
    }
    int sum = 0;
    for(int i = 0; i < 24; i++){
        sum -= piece_val[i] * get_population_count(position->bits[i]);
        out.score[i / 6] += 2 * piece_val[i] * get_population_count(position->bits[i]);
    }
    for(int i = 0; i < 4; i++){
        out.score[i] += sum;
        out.score[i] += 10000 * get_population_count(position->bits[i * 6 + K1]);
        out.score[i] -= 9568;
    }
    return out;
}
static inline int evaluate_position_simple_for_player(Position* position, enum Player ally){
    int out = 0;
    for(int i = 0; i < 24; i++){
        out -= piece_val[i] * get_population_count(position->bits[i]);
    }
    for(int i = 0; i < 6; i++){
        out += 2 * piece_val[i + ally * 6] * get_population_count(position->bits[i + ally * 6]);
    }
    out += 10000 * get_population_count(position->bits[ally * 6 + K1]);
    out -= 9568;
    return out;
}
void make_move(Position* position, U16 move){
    position->side++;
    if(!move){
        return;
    }
    int source;
    int dest;
    m_decode_move(source, dest, move);
    for(int i = 0; i < 24; i++){
        m_pop_bit(position->bits[i], dest);
    }
    for(int i = 0; i < 24; i++){
        if(m_get_bit(position->bits[i], source)){
            m_pop_bit(position->bits[i], source);
            if(i % 6 == P1 && m_get_bit(queening_masks[i / 6], dest)) m_set_bit(position->bits[i + Q1 - P1], dest);
            else m_set_bit(position->bits[i], dest);
        }
    }
}
int end_game_if_finished(Position* position){
    if(get_population_count(position->bits[K1] | position->bits[K2] | position->bits[K3] | position->bits[K4]) == 1){
        for(int i = 0; i < 4; i++){
            if(position->bits[K1 + i * 6]){
                printf("%sPlayer %s won!%s\n",colors[i + 1], unicode_pieces[5], colors[0]);
            }
        }
        return 1;
    }
    return 0;
}
int paranoid_alpha_beta(Position* position, enum Player player, int alpha, int beta, int root, int ply, U16* best_mov, int* nodes){
    if(ply <= 0){
        (*nodes)++;
        return evaluate_position_simple_for_player(position, player);
    }

    Move_tab tab = move_generator(position);
    Position copy;
    if(tab.count == 0){
        if(root){
            *best_mov = 0;
            return evaluate_position_simple_for_player(position, player);
        }
        else{
            copy = *position;
            make_move(&copy, 0);
            return paranoid_alpha_beta(&copy, player, alpha, beta, 0,  ply - 1, best_mov, nodes);
        }
    }

    int ply_score, temp;
    if(position->side % 4 == player){
        ply_score = -0xffffff;
        for(int i = 0; i < tab.count; i++){
            copy = *position;
            make_move(&copy, tab.moves[i]);
            temp = paranoid_alpha_beta(&copy, player, alpha, beta, 0,  ply - 1, best_mov, nodes);
            if(ply_score < temp){
                ply_score = temp;
                if(root){
                    *best_mov = tab.moves[i];
                }
            }
            if(ply_score > beta){
                break;
            }
            alpha = max(alpha, ply_score);
        }
    }
    else{
        ply_score = 0xffffff;
        for(int i = 0; i < tab.count; i++){
            copy = *position;
            make_move(&copy, tab.moves[i]);
            temp = paranoid_alpha_beta(&copy, player, alpha, beta, 0,  ply - 1, best_mov, nodes);
            if(ply_score > temp){
                ply_score = temp;
            }
            if(ply_score < alpha){
                break;
            }
            beta = min(beta, ply_score);
        }
    }

    
    return ply_score;
}
void query_engine_for_move(Position* position){
    int nodes = 0;
    int ply = 0;
    U16 move;
    int alpha = -0xffffff;
    int beta = 0xffffff;
    while(nodes < 0x4ffff && ply < 100){
    //while(ply < 6){
        nodes = 0;
        paranoid_alpha_beta(position, position->side % 4, alpha, beta, 1, ply, &move, &nodes);
        printf("%d: %d\n", ply, nodes);
        ply++;
    }
    make_move(position, move);
}
void read_move(U16 word){
    int source;
    int dest;
    m_decode_move(source, dest, word);
    printf("%s%s\n", index_to_coord[source], index_to_coord[dest]);
}
U16 parse_move(){
    int source;
    int dest;
    char buff[100];
    U16 move;
    fgets(buff, sizeof(buff), stdin);
    source = (int)(buff[0] - 'a') + 8 * (7 - (int)buff[1] + '1');
    dest = (int)(buff[2] - 'a') + 8 * (7 - (int)buff[3] + '1');
    move = m_encode_move(source, dest);
    return move;
}
void query_player_for_move(Position* position){
    Move_tab tab = move_generator(position);
    while(1){
        U16 move = parse_move();
        if(tab.count == 0){
            printf("\nthere is no possible move you dummy\n");
            make_move(position, 0);
            return;
        }
        for(int i = 0; i < tab.count; i++){
            if(move == tab.moves[i]){
                make_move(position, move);
                return;
            }
        }
        print_position(position);
        printf("\nhere are possible moves you dummy\n");
        for(int i = 0; i < tab.count; i++){
            read_move(tab.moves[i]);
        }
    }
}
int main(){
    Position position;
    memset(&position, 0, sizeof(Position));
    set_standard_position(&position);
    print_position(&position);

    enum Player_type players[] = {Human, Computer, Computer, Computer};

    while(1){
        for(int i = 0; i < 4; i++){
            print_score(evaluate_position_simple(&position));
            if(players[i] == Human){
                query_player_for_move(&position);
            }
            else{
                query_engine_for_move(&position);
            }
            print_position(&position);
            if(end_game_if_finished(&position)) return;
        }    
    }
    return 0;
}