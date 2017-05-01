/*
Copyright (c) 2016, oasi-adamay
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of glsCV nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*!
@file	pentomino.cpp
@author	oasi-adamay
@date	2016-5-29
@brief	ペントミノパズルの解を求める。
*/

//-----------------------------------------------------------------------------
// generic include
#include <stdio.h>
#include <string>
#include <string.h>
#include <climits>
#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <memory>
#include <chrono>
#include <assert.h>

#include "getopt.h"

using namespace std;

//-----------------------------------------------------------------------------
/*!
timer for process time measurement
*/
class Timer {
private:
	std::chrono::system_clock::time_point start;
	std::chrono::system_clock::time_point end;
	string msg;
public:
	Timer(string _msg) { msg = _msg;  Start(); }
	~Timer(void) { Stop(); std::cout << msg << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "[ms]" << std::endl; }
	void Start(void) { start = std::chrono::system_clock::now(); }
	void Stop(void) { end = std::chrono::system_clock::now(); }

};


/*!
const & define macro
*/
#define END_OF_NODE -1					//end of node
#define BOARD_CELL_EMPTY  -1			//cell is empty 
#define BOARD_CELL_BOARDER  INT_MAX		//cell is boarder 
#define PIECE_BLOCK_NUM 5				//piece block num (pentomino==5)

/*!
global var. (for debug)
*/
int g_find_solution_call_num = 0;	//!< total number of  calling function.

//-----------------------------------------------------------------------------

/*!
pentomino database struct
*/
struct pentomino_database_t{
	char      name;		//!< piece name (this naming convention is used in wikipedia. https://en.wikipedia.org/wiki/Pentomino)
	int       rows;		//!< rows of shape data
	int       cols;		//!< cols of shape data
	char	  data[9];	//!< piece shape data (to decrease  number of row, rotate the shape.)
	char      color[32];//!< color code
	
} database[] = {
	{
		'X', 3 , 3,
		{
			0,1,0,
			1,1,1,
			0,1,0,
		},
		"\x1b[41m\x1b[37m"	//red white
	},
	{
		'U', 2, 3,
		{
			1,1,1,
			1,0,1,
		},
		"\x1b[42m\x1b[37m"	//green white
	},
	{
		'W', 3, 3,
		{
			1,1,0,
			0,1,1,
			0,0,1,
		},
		"\x1b[43m\x1b[30m"	//yellow bk
	},
	{
		'F', 3, 3,
		{
			1,1,0,
			0,1,1,
			0,1,0,
		},
		"\x1b[44m\x1b[37m"	//blue white
	},
	{
		'Z', 3, 3,
		{
			1,1,0,
			0,1,0,
			0,1,1,
		},
		"\x1b[45m\x1b[30m"	//magenta bk
	},
	{
		'P', 2,3,
		{
			1,1,1,
			1,1,0,
		},
		"\x1b[46m\x1b[30m"	//cy bk
	},
	{
		'N', 2, 4,
		{
			1,1,1,0,
			0,0,1,1,
		},
		"\x1b[40m\x1b[31m"	//bk red
	},
	{
		'Y', 2, 4,
		{
			1,1,1,1,
			0,1,0,0,
		},
		"\x1b[40m\x1b[32m"	//bk green
	},
	{
		'T', 3,3,
		{
			1,1,1,
			0,1,0,
			0,1,0,
		},
		"\x1b[40m\x1b[33m"	//bk y
	},
	{
		'L', 2, 4,
		{
			1,1,1,1,
			1,0,0,0,
		},
		"\x1b[40m\x1b[34m"	//bk blue
	},
	{
		'V', 3, 3,
		{
			1,1,1,
			1,0,0,
			1,0,0,
		},
		"\x1b[40m\x1b[35m"	//bk mg
	},
	{
		'I', 1, 5,
		{
			1,1,1,1,1,
		},
		"\x1b[40m\x1b[36m"	//bk cy
	},
};


/*!
pentomino piece struct
*/
typedef struct {
	char	name;				//!< piece name
	char	color[32];			//!< piece color
	int    shape_num;			//!< shape num
	struct {
		//!< the offset address from shape origin(left top) for each block positions.
		//!< note: the stride depends on board size.
		int offsets[PIECE_BLOCK_NUM];
	} shape[8];					//!< the shape data for rotate/flip pieces.
} Piece;


//-----------------------------------------------------------------------------
// private functions

/*!
ボードの生成と初期化

rows+1 , cols+1のサイズの密な一次元配列のボードを生成する。
ボードの境界部は、BOARD_CELL_BOARDERで埋められ、ボードの内部は、BOARD_CELL_EMPTYで初期化される。

@param rows : ボードの行数
@param cols : ボードの列数
@retrun 初期化されたボード配列 int[(rows + 1)*(cols + 1)]
*/
static vector<int> create_board(int rows, int cols){
	vector<int> board((rows + 1)*(cols + 1));
	int* _board = &board[0];

	for (int y = 0; y < rows + 1; y++){
		for (int x = 0; x < cols + 1; x++){
			if (y < rows && x < cols){
				*_board++ = BOARD_CELL_EMPTY;
			}
			else{
				*_board++ = BOARD_CELL_BOARDER;
			}
		}
	}

	return board;
}

/*!
ボードの表示
*/
static void print_board(const vector<Piece>& pieces, const vector<int> board, const int rows, const int cols, const bool swap_ij)
{
	if (swap_ij) {
		for (int x = 0; x < cols; x++) {
			for (int y = 0; y < rows; y++) {
				int idx = y * (cols + 1) + x;
				const int n = board[idx];
				if (n != BOARD_CELL_EMPTY) {
					printf("%s%c", pieces[n].color,pieces[n].name);
 	
				}
				else {
					printf("%c", ' ');
				}
			}
			printf("\n");
		}
		printf("\n");
	}
	else {
		for (int y = 0; y < rows; y++) {
			for (int x = 0; x < cols; x++) {
				int idx = y * (cols + 1) + x;
				const int n = board[idx];
				if (n != BOARD_CELL_EMPTY) {
					printf("%s%c", pieces[n].color,pieces[n].name);
				}
				else {
					printf("%c", ' ');
				}
			}
			printf("\n");
		}
		printf("\n");
	}
#ifndef WIN32 
	printf("%s", "\x1b[49m\x1b[39m");
#endif

}



/*!
冗長な形状の削除。
remove redundant shape.

ペントミノ配置の全ての解の内、回転・鏡像による解は同一とした場合、
冗長な解、探索を削除するために、冗長な形状を削除する。
ペントミノ形状の内、回転・鏡像による相似がないピースを一つ選択し、
そのピースに関して、回転・鏡像による形状を削除する。
*/
static void remove_redundant_shape(vector<Piece>& pieces){
	for (int i = 0; i < (int)pieces.size(); i++){
		if (pieces[i].shape_num == 8){
			pieces[i].shape_num = 2;
			break;
		}
	}
}

static void sort_pieces_by_shape_num(vector<Piece>& pieces){
	class _myT{
	public:
		int idx;
		int val;
		_myT(int _idx, int _val){ idx = _idx; val = _val; }
		static bool compByVal(const _myT& a, const _myT& b) { return (a.val < b.val); }
	};

	list<_myT> lst;
	for (int i = 0; i<(int)pieces.size(); i++) lst.push_back(_myT(i, pieces[i].shape_num));
	lst.sort(_myT::compByVal);
//	lst.reverse();

	vector<Piece> tmp = pieces;
	int i = 0;
	for (auto itr = lst.begin(); itr != lst.end(); ++itr,i++) {
		pieces[i] = tmp[(*itr).idx];
	}
}


static void rotate_flip_piece(const char src[], char dst[], int& rows, int& cols, int code) {

	int _cols = cols;
	int _rows = rows;

	if (code % 2) {
		cols = _rows;
		rows = _cols;
	}

	switch (code) {
	case(0):	 //not rotated
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				int _i = i;	int _j = j;
				dst[i*cols + j] = src[_i*_cols + _j];
			}
		}
		break;
	case(1):	//rotated 90 deg
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				int _i = j;	int _j = i;
				dst[(rows - i - 1)*cols + j] = src[_i*_cols + _j];
			}
		}
		break;
	case(2): 	 //rotated 180 deg
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				int _i = i;	int _j = j;
				dst[(rows - i - 1)*cols + (cols - j - 1)] = src[_i*_cols + _j];
			}
		}
		break;
	case(3):   //rotated 270 deg
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				int _i = j;	int _j = i;
				dst[i*cols + (cols - j - 1)] = src[_i*_cols + _j];
			}
		}
		break;
	case(4):	 //not rotated + flip updown 
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				int _i = i;	int _j = j;
				dst[(rows - i - 1)*cols + j] = src[_i*_cols + _j];
			}
		}
		break;
	case(5):	//rotated 90 deg + flip updown
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				int _i = j;	int _j = i;
				dst[i*cols + j] = src[_i*_cols + _j];
			}
		}
		break;
	case(6): 	 //rotated 180 deg + flip updown
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				int _i = i;	int _j = j;
				dst[i*cols + (cols - j - 1)] = src[_i*_cols + _j];
			}
		}
		break;
	case(7):   //rotated 270 deg + flip updown
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				int _i = j;	int _j = i;
				dst[(rows - i - 1)*cols + (cols - j - 1)] = src[_i*_cols + _j];
			}
		}
		break;
	}
}



/*!
ペントミノの初期化
initialize piece data.

@param pieces: pentomino piece data.
@param rows: rows of the board to place the pentomino pieces.
@param cols: columns of the board to place the pentomino pieces.
*/
static void init_pieces(vector<Piece>& pieces, int rows, int cols){

	const int num = sizeof(database) / sizeof(database[0]);

	pieces.resize(num);

	for (int i = 0; i < num; i++){
		pentomino_database_t* db = &database[i];
		pieces[i].name = db->name;
#ifndef WIN32 
		strncpy(pieces[i].color,db->color,sizeof(pieces[i].color));
#endif
		int j = 0;
		char shape[sizeof(db->data)];

		// flip / rotate
		for (int z = 0; z < 8; z++){
			int shape_rows = db->rows;
			int shape_cols = db->cols;

			rotate_flip_piece(db->data, shape, shape_rows, shape_cols, z);
			
			//Excluded shape if  can not fit on board.
			if (shape_cols > cols || shape_rows > rows ) continue;

			//draw shape to temporary buffer
			vector<char> buff((rows + 1)*(cols + 1),0);
			for (int i = 0; i < shape_rows; i++) {
				for (int j = 0; j < shape_cols; j++) {
					buff[i*(cols + 1) + j] = shape[i*shape_cols + j];
				}
			}

			char *ptr = &buff[0];
			while (*ptr == 0){ ptr++; }		//skip until find first non-zero.(empty cell)
			int offset = 0;

			int offsets[PIECE_BLOCK_NUM];
			for (int k = 0; k < PIECE_BLOCK_NUM; k++){
				while (*ptr == 0){ ptr++,offset++; }	//skip until find first non-zero.
				offsets[k] = offset;
				ptr++, offset++;
			}

			//duplicate check with the registered data
			bool alrady_regist = false;
			for (int j2 = 0; j2 < pieces[i].shape_num; j2++){
				bool is_same = true;
				for (int k = 0; k < PIECE_BLOCK_NUM; k++){
					if (pieces[i].shape[j2].offsets[k] != offsets[k]){
						is_same = false;
						break;
					}
				}
				if (is_same) {
					alrady_regist = true;
					break;
				}
			}

			//store shape date
			if (!alrady_regist){
				for (int k = 0; k < PIECE_BLOCK_NUM; k++){
					pieces[i].shape[j].offsets[k] = offsets[k];
				}
				pieces[i].shape_num++;
				j++;
			}

		}

	}

	remove_redundant_shape(pieces);
	sort_pieces_by_shape_num(pieces);
}


/*!
現在のノード、現在のボード位置における解の探索

現在のノード位置（current_node）が、最後のノードならば、探索完了。解をリストに追加して、return する。 
現在のボード上の位置（current_board）に、未配置のピースが配置できるかチェックし配置する。
配置できたら、ノードを進めて、再帰コールで次のノードを探索する。

@param pieces			ペントミノデータ配列
@param used				ペントミノが使用済かを表すフラグ配列
@param current_board	現在のボード上の位置
@param current_node		現在の探査ノード
@param board			ボード
@param solution			解のリスト
@param find_all			全ての解を見つけるか?

@retuen ture:解を見つけた
*/
static bool find_solution(
	const vector<Piece>& pieces,
	vector<bool>& used,
	int* current_board,
	int* current_node,
	const vector<int>&board,
	list<vector<int>>& solution,
	const bool find_all,
	const int stride_num
)
{
#ifdef _OPENMP
#pragma omp atomic
#endif
	g_find_solution_call_num++;			//この操作は複数のスレッドから操作される

	const int n = (int)used.size();

	//未使用のピースの全てを探索
	for (int i = 0; i < n; i++){
		if (used[i]) { continue; }

		const Piece* piece = &pieces[i];

		//全ての配置で探索
		for (int j = 0; j < piece->shape_num; j++){
			const int* offset = &(piece->shape[j].offsets[0]);

			//ピースが置けるかチェックする。
			{
				bool can_place = true;
				for (int k = /*0*/ 1; k<PIECE_BLOCK_NUM; k++){		//k=0は自明
					if (current_board[offset[k]] != BOARD_CELL_EMPTY) { can_place = false;  break; }
				}
				if (!can_place)	continue;
			}

			
			{	//更新　（ここでの操作は、下の修復と対になる)
				for (int k = 0; k<PIECE_BLOCK_NUM; k++){ current_board[offset[k]] = i; }	// ピースを置く
				used[i] = true;				//使用済みフラグの更新
				*current_node++ = i;		//current_nodeの更新
			}

			//配置完了条件
			//current_nodeが最後に到達したら、解が見つかったことになる。
			if (*(current_node) == END_OF_NODE){
#ifdef _OPENMP
#pragma omp critical			
#endif
				{	solution.push_back(board);	}		//解をリストにコピーする。複数のスレッドで発生するのでcriticalで囲む
				
				if (!find_all)	return true;			//　単一解であれば、即時return
			}
			else{
				int* current_board_save = current_board;						//current_boardの更新の前に保存しておく
				while (*current_board != BOARD_CELL_EMPTY){	current_board++;}	//current_boardの更新
#if 1
				//全てのピース、配置で共通な除外条件をチェック
				//もっと積極的な枝狩でも良いが、条件チェックが重いと枝狩の恩恵が薄くなる。
				if (current_board[1] == BOARD_CELL_EMPTY ||	current_board[stride_num] == BOARD_CELL_EMPTY)
#endif
				{	//再帰
					if (find_solution(pieces, used, current_board, current_node, board, solution, find_all, stride_num)){
						if (!find_all)	return true;
					}
				}
				current_board = current_board_save;		//保存しておいたcurrent_boardで修復する
			}

			{	//修復　restore
				for (int k = 0; k<PIECE_BLOCK_NUM; k++){ current_board[offset[k]] = BOARD_CELL_EMPTY; }	// ピースを戻す
				used[i] = false;
				current_node--;
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// public functions

/*!
ペントミノの解を見つける。

@param rows 配置するボードの行数
@param cols 配置するボードの列数
@param find_all 全ての解を見つけるか?
@param print_all 全ての解を表示するか?

@return 解の数
*/
int solve_pentomino(int rows, int cols, const bool find_all, const bool print_all, const bool swap_ij){
	assert(rows > 0);
	assert(cols > 0);

	const int piece_num = sizeof(database) / sizeof(database[0]);

	//自明な条件の削除	
	if (piece_num*PIECE_BLOCK_NUM > (rows*cols)) return 0;

	vector<Piece> pieces;
	init_pieces(pieces, rows, cols);

	vector<bool> used(piece_num, false);

	vector<int>  node(piece_num + 1, 0);	//
	node[piece_num] = END_OF_NODE;			// nodeの終端を表す。
	int* current_node = &(node[0]);

	vector<int>  board = create_board(rows, cols);
	int* current_board = &(board[0]);

	list<vector<int>> solution;
	{
		Timer tmr("process time:\t");
		find_solution(pieces, used, current_board, current_node, board, solution, find_all,cols+1);
	}

	int solution_num = (int)solution.size();

	{	//解の表示
		int i = 0;

		for (auto itr = solution.begin(); itr != solution.end(); ++itr, i++) {
			vector<int> board = *itr;

			printf("#%d\n", i+1);
			print_board(pieces, board, rows, cols, swap_ij);

			if (!print_all) break;
		}
	}

	return solution_num;
}

/*!
ペントミノの解を見つける。(OpenMPによる並列探索)

@param rows 配置するボードの行数
@param cols 配置するボードの列数
@param find_all 全ての解を見つけるか?
@param print_all 全ての解を表示するか?

@return 解の数
*/
int solve_pentomino_omp(int rows, int cols, const bool find_all, const bool print_all, const bool swap_ij){
	assert(rows > 0);
	assert(cols > 0);

	const int piece_num = sizeof(database) / sizeof(database[0]);

	//自明な条件の削除	
	if (piece_num*PIECE_BLOCK_NUM > (rows*cols)) return 0;

	vector<Piece> pieces;
	init_pieces(pieces, rows, cols);

	int exit_frag = 0;
	const int stride_num = cols + 1;


	list<vector<int>> solution;
	{
		Timer tmr("process time:\t");

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
		for (int i = 0; i < piece_num; i++){

			//他スレッドで解を見つけた場合 
			if (exit_frag) continue;

			//thread local 
			vector<bool> used(piece_num, false);

			vector<int>  node(piece_num + 1, 0);	//
			node[piece_num] = END_OF_NODE;			// nodeの終端を表す。
			int* current_node = &(node[0]);

			vector<int>  board = create_board(rows, cols);
			int* current_board = &(board[0]);

			const Piece* piece = &pieces[i];

			//全ての配置で探索
			for (int j = 0; j < piece->shape_num; j++){
				if (exit_frag) continue;
				const int* offset = &(piece->shape[j].offsets[0]);

				//ピースが置けるかチェックする。
				bool can_place = true;
				for (int k = 0; k<PIECE_BLOCK_NUM; k++){
					if (current_board[offset[k]] != BOARD_CELL_EMPTY) { can_place = false;  break; }
				}
				if (!can_place)	continue;

				{	//更新　（ここでの操作は、下の修復と対になる)
					for (int k = 0; k<PIECE_BLOCK_NUM; k++){ current_board[offset[k]] = i; }	// ピースを置く
					used[i] = true;				//使用済みフラグの更新
					*current_node++ = i;		//current_nodeの更新
				}

				{
					int* current_board_save = current_board;						//current_boardの更新の前に保存しておく
					while (*current_board != BOARD_CELL_EMPTY){ current_board++; }	//current_boardの更新
#if 1
					//全てのピース、配置で共通な除外条件をチェック
					//もっと積極的な枝狩でも良いが、条件チェックが重いと枝狩の恩恵が薄くなる。
					if (current_board[1] == BOARD_CELL_EMPTY || current_board[stride_num] == BOARD_CELL_EMPTY)
#endif
					{	//再帰
						if (find_solution(pieces, used, current_board, current_node, board, solution, find_all, stride_num)){
							if (!find_all)	{
#ifdef _OPENMP
#pragma omp atomic
#endif
								exit_frag++;
								if (exit_frag) continue;
							}
						}
					}
					current_board = current_board_save;		//保存しておいたcurrent_boardで修復する
				}

				{	//修復　restore
					for (int k = 0; k<PIECE_BLOCK_NUM; k++){ current_board[offset[k]] = BOARD_CELL_EMPTY; }	// ピースを戻す
					used[i] = false;
					current_node--;
				}
			}
		}
	}

	int solution_num = (int)solution.size();

	{	//解の表示
		int i = 0;

		for (auto itr = solution.begin(); itr != solution.end(); ++itr, i++) {
			vector<int> board = *itr;

			printf("#%d\n", i + 1);
			print_board(pieces, board, rows, cols, swap_ij);

			if (!print_all) break;
		}
	}

	return solution_num;
}


//-----------------------------------------------------------------------------
// main application


void usage(void) {
	printf(
	"usage:	pentomino [-r rows] [-c cols ] [-fpm]""\n"
	"-r rows of the board to place the pentomino pieces.""\n"
	"-c cols of the board to place the pentomino pieces.""\n"
	"-f find all solutions.""\n"
	"-p print all solutions.(otherwise, print only first founded solution.)""\n"
	"-m find solutions usinhg openmp.""\n"
	);
}

int main(int argc, char* argv[]){

	int opt;
	int rows = 6;
	int cols = 10;
	bool find_all = true;
	bool print_all = false;
	bool use_openmp = false;

	while ((opt = getopt(argc, argv, "r:c:fpmh?")) != -1) {
		switch (opt) {
		case 'r':
			rows = atoi(optarg);
			break;
		case 'c':
			cols = atoi(optarg);
			break;
		case 'f':
			find_all = true;
			break;
		case 'p':
			print_all = true;
			break;
		case 'm':
			use_openmp = true;
			break;
		case '?':
		case 'h':
		default: /* '?' */
			usage();
			exit(EXIT_FAILURE);
		}
	}


	{
		Timer tmr("total time:\t");

		// arg check
		if (rows < 1 || cols < 1){
			printf("rows and clos must be larger than 0.\n");
			exit(EXIT_FAILURE);
		}

		//print args..
		printf("board rows:%d\tcols:%d\n", rows, cols);
		printf("find_all:%d\n", find_all);
		printf("print_all:%d\n", print_all);
		printf("use_openmp:%d\n", use_openmp);

		//clear globla val..
		g_find_solution_call_num = 0;

		//探索方向が横方向の為、縦長のboardの方が、効率よく枝狩り出来る。
		bool swap_ij = false;
		if (cols > rows) {
			std::swap(rows, cols);
			swap_ij = true;
		}

		int solution_num;		//total solution num

		if (use_openmp){
			solution_num = solve_pentomino_omp(rows, cols, find_all, print_all, swap_ij);
		}
		else{
			solution_num = solve_pentomino(rows, cols, find_all, print_all, swap_ij);
		}


		printf("solution_num: %d\n", solution_num);
		printf("find_solution_call_num : %d\n", g_find_solution_call_num);
	}

//	printf("Hit return key.\n");
//	cin.get();
	return 0;
}

