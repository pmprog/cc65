/*****************************************************************************/
/*                                                                           */
/*			 	   nexttok.c				     */
/*                                                                           */
/*		Get next token and handle token level functions		     */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 2000     Ullrich von Bassewitz                                        */
/*              Wacholderweg 14                                              */
/*              D-70597 Stuttgart                                            */
/* EMail:       uz@musoftware.de                                             */
/*                                                                           */
/*                                                                           */
/* This software is provided 'as-is', without any expressed or implied       */
/* warranty.  In no event will the authors be held liable for any damages    */
/* arising from the use of this software.                                    */
/*                                                                           */
/* Permission is granted to anyone to use this software for any purpose,     */
/* including commercial applications, and to alter it and redistribute it    */
/* freely, subject to the following restrictions:                            */
/*                                                                           */
/* 1. The origin of this software must not be misrepresented; you must not   */
/*    claim that you wrote the original software. If you use this software   */
/*    in a product, an acknowledgment in the product documentation would be  */
/*    appreciated but is not required.                                       */
/* 2. Altered source versions must be plainly marked as such, and must not   */
/*    be misrepresented as being the original software.                      */
/* 3. This notice may not be removed or altered from any source              */
/*    distribution.                                                          */
/*                                                                           */
/*****************************************************************************/



#include <stdio.h>

#include "error.h"
#include "expr.h"
#include "scanner.h"
#include "toklist.h"
#include "nexttok.h"



/*****************************************************************************/
/*     	       	    		     Data			   	     */
/*****************************************************************************/



static unsigned RawMode = 0;		/* Raw token mode flag/counter */



/*****************************************************************************/
/*     	       	    		     Code			   	     */
/*****************************************************************************/



static TokList* CollectTokens (unsigned Start, unsigned Count)
/* Read a list of tokens that is terminated by a right paren. For all tokens
 * starting at the one with index Start, and ending at (Start+Count-1), place
 * them into a token list, and return this token list.
 */
{
    /* Create the token list */
    TokList* List = NewTokList ();

    /* Read the token list */
    unsigned Current = 0;
    unsigned Parens  = 0;
    while (Parens != 0 || Tok != TOK_RPAREN) {

     	/* Check for end of line or end of input */
     	if (Tok == TOK_SEP || Tok == TOK_EOF) {
     	    Error (ERR_UNEXPECTED_EOL);
     	    return List;
     	}

     	/* Collect tokens in the given range */
     	if (Current >= Start && Current < Start+Count) {
     	    /* Add the current token to the list */
     	    AddCurTok (List);
     	}

     	/* Check for and count parenthesii */
     	if (Tok == TOK_LPAREN) {
     	    ++Parens;
     	} else if (Tok == TOK_RPAREN) {
     	    --Parens;
     	}

     	/* Get the next token */
     	++Current;
     	NextTok ();
    }

    /* Eat the closing paren */
    ConsumeRParen ();

    /* Return the list of collected tokens */
    return List;
}



static void FuncConcat (void)
/* Handle the .CONCAT function */
{
    char    	Buf[MAX_STR_LEN+1];
    char*	B;
    unsigned 	Length;
    unsigned	L;

    /* Skip it */
    NextTok ();

    /* Left paren expected */
    ConsumeLParen ();

    /* Concatenate any number of strings */
    B = Buf;
    B[0] = '\0';
    Length = 0;
    while (1) {

     	/* Next token must be a string */
     	if (Tok != TOK_STRCON) {
     	    Error (ERR_STRCON_EXPECTED);
     	    SkipUntilSep ();
     	    return;
     	}

     	/* Get the length of the string const and check total length */
     	L = strlen (SVal);
     	if (Length + L > MAX_STR_LEN) {
     	    Error (ERR_STRING_TOO_LONG);
     	    /* Try to recover */
     	    SkipUntilSep ();
     	    return;
     	}

     	/* Add the new string */
     	memcpy (B, SVal, L);
     	Length += L;
     	B      += L;

     	/* Skip the string token */
     	NextTok ();

     	/* Comma means another argument */
     	if (Tok == TOK_COMMA) {
     	    NextTok ();
     	} else {
     	    /* Done */
     	    break;
     	}
    }

    /* Terminate the string */
    *B = '\0';

    /* We expect a closing parenthesis, but will not skip it but replace it
     * by the string token just created.
     */
    if (Tok != TOK_RPAREN) {
     	Error (ERR_RPAREN_EXPECTED);
    } else {
     	Tok = TOK_STRCON;
     	strcpy (SVal, Buf);
    }
}



static void FuncLeft (void)
/* Handle the .LEFT function */
{
    long       	Count;
    TokList* 	List;

    /* Skip it */
    NextTok ();

    /* Left paren expected */
    ConsumeLParen ();

    /* Count argument */
    Count = ConstExpression ();
    if (Count < 0 || Count > 100) {
     	Error (ERR_RANGE);
     	Count = 1;
    }
    ConsumeComma ();

    /* Read the token list */
    List = CollectTokens (0, (unsigned) Count);

    /* Since we want to insert the list before the now current token, we have
     * to save the current token in some way and then skip it. To do this, we
     * will add the current token at the end of the token list (so the list
     * will never be empty), push the token list, and then skip the current
     * token. This will replace the current token by the first token from the
     * list (which will be the old current token in case the list was empty).
     */
    AddCurTok (List);

    /* Insert it into the scanner feed */
    PushTokList (List, ".LEFT");

    /* Skip the current token */
    NextTok ();
}



static void FuncMid (void)
/* Handle the .MID function */
{
    long    	Start;
    long       	Count;
    TokList* 	List;

    /* Skip it */
    NextTok ();

    /* Left paren expected */
    ConsumeLParen ();

    /* Start argument */
    Start = ConstExpression ();
    if (Start < 0 || Start > 100) {
     	Error (ERR_RANGE);
     	Start = 0;
    }
    ConsumeComma ();

    /* Count argument */
    Count = ConstExpression ();
    if (Count < 0 || Count > 100) {
     	Error (ERR_RANGE);
     	Count = 1;
    }
    ConsumeComma ();

    /* Read the token list */
    List = CollectTokens ((unsigned) Start, (unsigned) Count);

    /* Since we want to insert the list before the now current token, we have
     * to save the current token in some way and then skip it. To do this, we
     * will add the current token at the end of the token list (so the list
     * will never be empty), push the token list, and then skip the current
     * token. This will replace the current token by the first token from the
     * list (which will be the old current token in case the list was empty).
     */
    AddCurTok (List);

    /* Insert it into the scanner feed */
    PushTokList (List, ".MID");

    /* Skip the current token */
    NextTok ();
}



static void FuncRight (void)
/* Handle the .RIGHT function */
{
    long       	Count;
    TokList* 	List;

    /* Skip it */
    NextTok ();

    /* Left paren expected */
    ConsumeLParen ();

    /* Count argument */
    Count = ConstExpression ();
    if (Count < 0 || Count > 100) {
     	Error (ERR_RANGE);
     	Count = 1;
    }
    ConsumeComma ();

    /* Read the complete token list */
    List = CollectTokens (0, 9999);

    /* Delete tokens from the list until Count tokens are remaining */
    while (List->Count > Count) {
	/* Get the first node */
	TokNode* T = List->Root;

	/* Remove it from the list */
	List->Root = List->Root->Next;

	/* Free the node */
	FreeTokNode (T);

	/* Corrent the token counter */
	List->Count--;
    }

    /* Since we want to insert the list before the now current token, we have
     * to save the current token in some way and then skip it. To do this, we
     * will add the current token at the end of the token list (so the list
     * will never be empty), push the token list, and then skip the current
     * token. This will replace the current token by the first token from the
     * list (which will be the old current token in case the list was empty).
     */
    AddCurTok (List);

    /* Insert it into the scanner feed */
    PushTokList (List, ".RIGHT");

    /* Skip the current token */
    NextTok ();
}



static void FuncString (void)
/* Handle the .STRING function */
{
    char Buf[MAX_STR_LEN+1];

    /* Skip it */
    NextTok ();

    /* Left paren expected */
    ConsumeLParen ();

    /* Accept identifiers or numeric expressions */
    if (Tok == TOK_IDENT) {
     	/* Save the identifier, then skip it */
     	strcpy (Buf, SVal);
     	NextTok ();
    } else {
     	/* Numeric expression */
     	long Val = ConstExpression ();
     	sprintf (Buf, "%ld", Val);
    }

    /* We expect a closing parenthesis, but will not skip it but replace it
     * by the string token just created.
     */
    if (Tok != TOK_RPAREN) {
     	Error (ERR_RPAREN_EXPECTED);
    } else {
     	Tok = TOK_STRCON;
     	strcpy (SVal, Buf);
    }
}



void NextTok (void)
/* Get next token and handle token level functions */
{
    /* Get the next raw token */
    NextRawTok ();

    /* In raw mode, pass the token unchanged */
    if (RawMode == 0) {

	/* Execute token handling functions */
	switch (Tok) {

	    case TOK_CONCAT:
		FuncConcat ();
		break;

	    case TOK_LEFT:
		FuncLeft ();
		break;

	    case TOK_MID:
		FuncMid ();
		break;

	    case TOK_RIGHT:
		FuncRight ();
		break;

	    case TOK_STRING:
		FuncString ();
		break;

	    default:
		/* Quiet down gcc */
		break;

	}
    }
}



void Consume (enum Token Expected, unsigned ErrMsg)
/* Consume Expected, print an error if we don't find it */
{
    if (Tok == Expected) {
	NextTok ();
    } else {
	Error (ErrMsg);
    }
}



void ConsumeSep (void)
/* Consume a separator token */
{
    /* Accept an EOF as separator */
    if (Tok != TOK_EOF) {
     	if (Tok != TOK_SEP) {
     	    Error (ERR_TOO_MANY_CHARS);
     	    SkipUntilSep ();
     	} else {
     	    NextTok ();
     	}
    }
}



void ConsumeLParen (void)
/* Consume a left paren */
{
    Consume (TOK_LPAREN, ERR_LPAREN_EXPECTED);
}



void ConsumeRParen (void)
/* Consume a right paren */
{
    Consume (TOK_RPAREN, ERR_RPAREN_EXPECTED);
}



void ConsumeComma (void)
/* Consume a comma */
{
    Consume (TOK_COMMA, ERR_COMMA_EXPECTED);
}



void SkipUntilSep (void)
/* Skip tokens until we reach a line separator or end of file */
{
    while (Tok != TOK_SEP && Tok != TOK_EOF) {
     	NextTok ();
    }
}



void EnterRawTokenMode (void)
/* Enter raw token mode. In raw mode, token handling functions are not
 * executed, but the function tokens are passed untouched to the upper
 * layer. Raw token mode is used when storing macro tokens for later
 * use.
 * Calls to EnterRawTokenMode and LeaveRawTokenMode may be nested.
 */
{
    ++RawMode;
}



void LeaveRawTokenMode (void)
/* Leave raw token mode. */
{
    PRECONDITION (RawMode > 0);
    --RawMode;
}



