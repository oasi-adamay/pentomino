#ifndef _GETOPT_H_
#define  _GETOPT_H_

extern int     opterr;       /* getopt()エラーメッセージ 1:表示 0:非表示 */
extern int     optind;       /* getopt()インデックス */
extern int     optopt;           /* 取得オプション文字 */
extern char    *optarg;          /* 取得オプションパラメータ文字列 */

extern int getopt(int ac, char **av, const char *opts);


#endif