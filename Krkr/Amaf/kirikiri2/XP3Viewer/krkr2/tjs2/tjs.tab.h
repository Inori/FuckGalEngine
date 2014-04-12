namespace TJS {
/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_COMMA = 258,
     T_EQUAL = 259,
     T_AMPERSANDEQUAL = 260,
     T_VERTLINEEQUAL = 261,
     T_CHEVRONEQUAL = 262,
     T_MINUSEQUAL = 263,
     T_PLUSEQUAL = 264,
     T_PERCENTEQUAL = 265,
     T_SLASHEQUAL = 266,
     T_BACKSLASHEQUAL = 267,
     T_ASTERISKEQUAL = 268,
     T_LOGICALOREQUAL = 269,
     T_LOGICALANDEQUAL = 270,
     T_RARITHSHIFTEQUAL = 271,
     T_LARITHSHIFTEQUAL = 272,
     T_RBITSHIFTEQUAL = 273,
     T_QUESTION = 274,
     T_LOGICALOR = 275,
     T_LOGICALAND = 276,
     T_VERTLINE = 277,
     T_CHEVRON = 278,
     T_AMPERSAND = 279,
     T_NOTEQUAL = 280,
     T_EQUALEQUAL = 281,
     T_DISCNOTEQUAL = 282,
     T_DISCEQUAL = 283,
     T_SWAP = 284,
     T_LT = 285,
     T_GT = 286,
     T_LTOREQUAL = 287,
     T_GTOREQUAL = 288,
     T_RARITHSHIFT = 289,
     T_LARITHSHIFT = 290,
     T_RBITSHIFT = 291,
     T_PERCENT = 292,
     T_SLASH = 293,
     T_BACKSLASH = 294,
     T_ASTERISK = 295,
     T_EXCRAMATION = 296,
     T_TILDE = 297,
     T_DECREMENT = 298,
     T_INCREMENT = 299,
     T_NEW = 300,
     T_DELETE = 301,
     T_TYPEOF = 302,
     T_PLUS = 303,
     T_MINUS = 304,
     T_SHARP = 305,
     T_DOLLAR = 306,
     T_ISVALID = 307,
     T_INVALIDATE = 308,
     T_INSTANCEOF = 309,
     T_LPARENTHESIS = 310,
     T_DOT = 311,
     T_LBRACKET = 312,
     T_THIS = 313,
     T_SUPER = 314,
     T_GLOBAL = 315,
     T_RBRACKET = 316,
     T_CLASS = 317,
     T_RPARENTHESIS = 318,
     T_COLON = 319,
     T_SEMICOLON = 320,
     T_LBRACE = 321,
     T_RBRACE = 322,
     T_CONTINUE = 323,
     T_FUNCTION = 324,
     T_DEBUGGER = 325,
     T_DEFAULT = 326,
     T_CASE = 327,
     T_EXTENDS = 328,
     T_FINALLY = 329,
     T_PROPERTY = 330,
     T_PRIVATE = 331,
     T_PUBLIC = 332,
     T_PROTECTED = 333,
     T_STATIC = 334,
     T_RETURN = 335,
     T_BREAK = 336,
     T_EXPORT = 337,
     T_IMPORT = 338,
     T_SWITCH = 339,
     T_IN = 340,
     T_INCONTEXTOF = 341,
     T_FOR = 342,
     T_WHILE = 343,
     T_DO = 344,
     T_IF = 345,
     T_VAR = 346,
     T_CONST = 347,
     T_ENUM = 348,
     T_GOTO = 349,
     T_THROW = 350,
     T_TRY = 351,
     T_SETTER = 352,
     T_GETTER = 353,
     T_ELSE = 354,
     T_CATCH = 355,
     T_OMIT = 356,
     T_SYNCHRONIZED = 357,
     T_WITH = 358,
     T_INT = 359,
     T_REAL = 360,
     T_STRING = 361,
     T_OCTET = 362,
     T_FALSE = 363,
     T_NULL = 364,
     T_TRUE = 365,
     T_VOID = 366,
     T_NAN = 367,
     T_INFINITY = 368,
     T_UPLUS = 369,
     T_UMINUS = 370,
     T_EVAL = 371,
     T_POSTDECREMENT = 372,
     T_POSTINCREMENT = 373,
     T_IGNOREPROP = 374,
     T_PROPACCESS = 375,
     T_ARG = 376,
     T_EXPANDARG = 377,
     T_INLINEARRAY = 378,
     T_ARRAYARG = 379,
     T_INLINEDIC = 380,
     T_DICELM = 381,
     T_WITHDOT = 382,
     T_THIS_PROXY = 383,
     T_WITHDOT_PROXY = 384,
     T_CONSTVAL = 385,
     T_SYMBOL = 386,
     T_REGEXP = 387
   };
#endif
/* Tokens.  */
#define T_COMMA 258
#define T_EQUAL 259
#define T_AMPERSANDEQUAL 260
#define T_VERTLINEEQUAL 261
#define T_CHEVRONEQUAL 262
#define T_MINUSEQUAL 263
#define T_PLUSEQUAL 264
#define T_PERCENTEQUAL 265
#define T_SLASHEQUAL 266
#define T_BACKSLASHEQUAL 267
#define T_ASTERISKEQUAL 268
#define T_LOGICALOREQUAL 269
#define T_LOGICALANDEQUAL 270
#define T_RARITHSHIFTEQUAL 271
#define T_LARITHSHIFTEQUAL 272
#define T_RBITSHIFTEQUAL 273
#define T_QUESTION 274
#define T_LOGICALOR 275
#define T_LOGICALAND 276
#define T_VERTLINE 277
#define T_CHEVRON 278
#define T_AMPERSAND 279
#define T_NOTEQUAL 280
#define T_EQUALEQUAL 281
#define T_DISCNOTEQUAL 282
#define T_DISCEQUAL 283
#define T_SWAP 284
#define T_LT 285
#define T_GT 286
#define T_LTOREQUAL 287
#define T_GTOREQUAL 288
#define T_RARITHSHIFT 289
#define T_LARITHSHIFT 290
#define T_RBITSHIFT 291
#define T_PERCENT 292
#define T_SLASH 293
#define T_BACKSLASH 294
#define T_ASTERISK 295
#define T_EXCRAMATION 296
#define T_TILDE 297
#define T_DECREMENT 298
#define T_INCREMENT 299
#define T_NEW 300
#define T_DELETE 301
#define T_TYPEOF 302
#define T_PLUS 303
#define T_MINUS 304
#define T_SHARP 305
#define T_DOLLAR 306
#define T_ISVALID 307
#define T_INVALIDATE 308
#define T_INSTANCEOF 309
#define T_LPARENTHESIS 310
#define T_DOT 311
#define T_LBRACKET 312
#define T_THIS 313
#define T_SUPER 314
#define T_GLOBAL 315
#define T_RBRACKET 316
#define T_CLASS 317
#define T_RPARENTHESIS 318
#define T_COLON 319
#define T_SEMICOLON 320
#define T_LBRACE 321
#define T_RBRACE 322
#define T_CONTINUE 323
#define T_FUNCTION 324
#define T_DEBUGGER 325
#define T_DEFAULT 326
#define T_CASE 327
#define T_EXTENDS 328
#define T_FINALLY 329
#define T_PROPERTY 330
#define T_PRIVATE 331
#define T_PUBLIC 332
#define T_PROTECTED 333
#define T_STATIC 334
#define T_RETURN 335
#define T_BREAK 336
#define T_EXPORT 337
#define T_IMPORT 338
#define T_SWITCH 339
#define T_IN 340
#define T_INCONTEXTOF 341
#define T_FOR 342
#define T_WHILE 343
#define T_DO 344
#define T_IF 345
#define T_VAR 346
#define T_CONST 347
#define T_ENUM 348
#define T_GOTO 349
#define T_THROW 350
#define T_TRY 351
#define T_SETTER 352
#define T_GETTER 353
#define T_ELSE 354
#define T_CATCH 355
#define T_OMIT 356
#define T_SYNCHRONIZED 357
#define T_WITH 358
#define T_INT 359
#define T_REAL 360
#define T_STRING 361
#define T_OCTET 362
#define T_FALSE 363
#define T_NULL 364
#define T_TRUE 365
#define T_VOID 366
#define T_NAN 367
#define T_INFINITY 368
#define T_UPLUS 369
#define T_UMINUS 370
#define T_EVAL 371
#define T_POSTDECREMENT 372
#define T_POSTINCREMENT 373
#define T_IGNOREPROP 374
#define T_PROPACCESS 375
#define T_ARG 376
#define T_EXPANDARG 377
#define T_INLINEARRAY 378
#define T_ARRAYARG 379
#define T_INLINEDIC 380
#define T_DICELM 381
#define T_WITHDOT 382
#define T_THIS_PROXY 383
#define T_WITHDOT_PROXY 384
#define T_CONSTVAL 385
#define T_SYMBOL 386
#define T_REGEXP 387




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 60 "tjs.y"
{
	tjs_int			num;
	tTJSExprNode *		np;
}
/* Line 1489 of yacc.c.  */
#line 318 "tjs.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif




}
