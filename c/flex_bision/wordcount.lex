%{
    int chars = 0;
    int words = 0;
    int lines = 0;
%}
%%
[^ \t\n\r\f\v]+ { words++; chars += strlen(yytext); }
\n { lines++; chars++; }
. {chars++;}
%%
main(int argc, char **argv)
{
    if (argc > 1) {
        if (!(yyin = fopen(argv[1], "r"))) {
            perror(argv[1]);
            return 1;
        }
    }
    yylex();
    printf("chars=%d, words=%d, lines=%d\n", chars, words, lines);
}
