#if !defined(MINIMAL) && !defined(OMIT_QCC)

#include "qcc.h"
#ifdef QCC
#define print printf
#endif
#include "time.h"

#define MEMBERFIELDNAME "__m%s"

#define STRCMP(s1,s2) (((*s1)!=(*s2)) || strcmp(s1+1,s2+1))	//saves about 2-6 out of 120 - expansion of idea from fastqcc

void QCC_PR_PreProcessor_Define(pbool append);
pbool QCC_PR_UndefineName(char *name);
char *QCC_PR_CheckCompConstString(char *def);
CompilerConstant_t *QCC_PR_CheckCompConstDefined(char *def);
int QCC_PR_CheckCompConst(void);
pbool QCC_Include(char *filename);
void QCC_FreeDef(QCC_def_t *def);

char *compilingfile;

int			pr_source_line;

char		*pr_file_p;
char		*pr_line_start;		// start of current source line

int			pr_bracelevel;

char		pr_token[8192];
token_type_t	pr_token_type;
int				pr_token_line;
int				pr_token_line_last;
QCC_type_t		*pr_immediate_type;
QCC_evalstorage_t		pr_immediate;

char	pr_immediate_string[8192];

int		pr_error_count;
int		pr_warning_count;


CompilerConstant_t *CompilerConstant;
int numCompilerConstants;
extern pbool expandedemptymacro;

//really these should not be in here
extern unsigned int locals_end, locals_start;
extern QCC_type_t *pr_classtype;
QCC_function_t *QCC_PR_ParseImmediateStatements (QCC_def_t *def, QCC_type_t *type);


static void Q_strlcpy(char *dest, const char *src, int sizeofdest)
{
	if (sizeofdest)
	{
		int slen = strlen(src);
		slen = min((sizeofdest-1), slen);
		memcpy(dest, src, slen);
		dest[slen] = 0;
	}
}



char	*pr_punctuation[] =
// longer symbols must be before a shorter partial match
{"&&", "||", "<=", ">=","==", "!=", "/=", "*=", "+=", "-=", "(+)", "(-)", "|=", "&~=", "&=", "++", "--", "->", "^=", "::", ";", ",", "!", "*", "/", "(", ")", "-", "+", "=", "[", "]", "{", "}", "...", "..", ".", "<<", "<", ">>", ">" , "?", "#" , "@", "&" , "|", "%", "^", "~", ":", NULL};

char *pr_punctuationremap[] =	//a nice bit of evilness.
//(+) -> |=
//-> -> .
//(-) -> &~=
{"&&", "||", "<=", ">=","==", "!=", "/=", "*=", "+=", "-=", "|=",  "&~=", "|=", "&~=", "&=", "++", "--", ".",  "^=", "::", ";", ",", "!", "*", "/", "(", ")", "-", "+", "=", "[", "]", "{", "}", "...", "..", ".", "<<", "<", ">>", ">" , "?", "#" , "@", "&" , "|", "%", "^", "~", ":", NULL};

// simple types.  function types are dynamically allocated
QCC_type_t	*type_void;				//void
QCC_type_t	*type_string;			//string
QCC_type_t	*type_float;			//float
QCC_type_t	*type_vector;			//vector
QCC_type_t	*type_entity;			//entity
QCC_type_t	*type_field;			//.void
QCC_type_t	*type_function;			//void()
QCC_type_t	*type_floatfunction;	//float()
QCC_type_t	*type_pointer;			//??? * - careful with this one
QCC_type_t	*type_integer;			//int
QCC_type_t	*type_variant;			//__variant
QCC_type_t	*type_floatpointer;		//float *
QCC_type_t	*type_intpointer;		//int *

QCC_type_t	*type_floatfield;// = {ev_field/*, &def_field*/, NULL, &type_float};

QCC_def_t	def_ret, def_parms[MAX_PARMS];

//QCC_def_t	*def_for_type[9] = {&def_void, &def_string, &def_float, &def_vector, &def_entity, &def_field, &def_function, &def_pointer, &def_integer};

void QCC_PR_LexWhitespace (pbool inhibitpreprocessor);




//for compiler constants and file includes.

qcc_includechunk_t *currentchunk;
void QCC_PR_CloseProcessor(void)
{
	currentchunk = NULL;
}
void QCC_PR_IncludeChunkEx (char *data, pbool duplicate, char *filename, CompilerConstant_t *cnst)
{
	qcc_includechunk_t *chunk = qccHunkAlloc(sizeof(qcc_includechunk_t));
	chunk->prev = currentchunk;
	currentchunk = chunk;

	chunk->currentdatapoint = pr_file_p;
	chunk->currentlinenumber = pr_source_line;
	chunk->cnst = cnst;
	if( cnst )
	{
		cnst->inside++;
	}

	if (duplicate)
	{
		pr_file_p = qccHunkAlloc(strlen(data)+1);
		strcpy(pr_file_p, data);
	}
	else
		pr_file_p = data;
}
void QCC_PR_IncludeChunk (char *data, pbool duplicate, char *filename)
{
	QCC_PR_IncludeChunkEx(data, duplicate, filename, NULL);
}

pbool QCC_PR_UnInclude(void)
{
	if (!currentchunk)
		return false;

	if( currentchunk->cnst )
		currentchunk->cnst->inside--;

	pr_file_p = currentchunk->currentdatapoint;
	pr_source_line = currentchunk->currentlinenumber;

	currentchunk = currentchunk->prev;

	return true;
}


/*
==============
PR_PrintNextLine
==============
*/
void QCC_PR_PrintNextLine (void)
{
	char	*t;

	printf ("%3i:",pr_source_line);
	for (t=pr_line_start ; *t && *t != '\n' ; t++)
		printf ("%c",*t);
	printf ("\n");
}

extern char qccmsourcedir[];
//also meant to include it.
void QCC_FindBestInclude(char *newfile, char *currentfile, char *rootpath, pbool verbose)
{
	char fullname[1024];
	int doubledots;

	char *end = fullname;

	if (!*newfile)
		return;

	doubledots = 0;
	/*count how far up we need to go*/
	while(!strncmp(newfile, "../", 3) || !strncmp(newfile, "..\\", 3))
	{
		newfile+=3;
		doubledots++;
	}

#if 0
	currentfile += strlen(rootpath);	//could this be bad?
	strcpy(fullname, rootpath);

	end = fullname+strlen(end);
	if (*fullname && end[-1] != '/')
	{
		strcpy(end, "/");
		end = end+strlen(end);
	}
	strcpy(end, currentfile);
	end = end+strlen(end);
#else
	if (currentfile)
		strcpy(fullname, currentfile);
	else
		*fullname = 0;
	end = fullname+strlen(fullname);
#endif

	while (end > fullname)
	{
		end--;
		/*stop at the slash, unless we're meant to go further*/
		if (*end == '/' || *end == '\\')
		{
			if (!doubledots)
			{
				end++;
				break;
			}
			doubledots--;
		}
	}

	strcpy(end, newfile);

	if (verbose)
	{
		if (autoprototype)
			printf("prototyping include %s\n", fullname);
		else
			printf("including %s\n", fullname);
	}
	QCC_Include(fullname);
}

pbool defaultnoref;
pbool defaultstatic;
int ForcedCRC;
int QCC_PR_LexInteger (void);
void	QCC_AddFile (char *filename);
void QCC_PR_LexString (void);
pbool QCC_PR_SimpleGetToken (void);

#define PPI_VALUE 0
#define PPI_NOT 1
#define PPI_DEFINED 2
#define PPI_COMPARISON 3
#define PPI_LOGICAL 4
#define PPI_TOPLEVEL 5
int ParsePrecompilerIf(int level)
{
	CompilerConstant_t *c;
	int eval = 0;
//	pbool notted = false;

	//single term end-of-chain
	if (level == PPI_VALUE)
	{
		/*skip whitespace*/
		while (*pr_file_p && qcc_iswhite(*pr_file_p) && *pr_file_p != '\n')
		{
			pr_file_p++;
		}

		if (*pr_file_p == '(')
		{	//try brackets
			pr_file_p++;
			eval = ParsePrecompilerIf(PPI_TOPLEVEL);
			while (*pr_file_p == ' ' || *pr_file_p == '\t')
				pr_file_p++;
			if (*pr_file_p != ')')
				QCC_PR_ParseError(ERR_EXPECTED, "unclosed bracket condition\n");
			pr_file_p++;
		}
		else if (*pr_file_p == '!')
		{	//try brackets
			pr_file_p++;
			eval = !ParsePrecompilerIf(PPI_NOT);
		}
		else
		{	//simple token...
			if (!strncmp(pr_file_p, "defined", 7))
			{
				pr_file_p+=7;
				while (*pr_file_p == ' ' || *pr_file_p == '\t')
					pr_file_p++;
				if (*pr_file_p != '(')
				{
					eval = false;
					QCC_PR_ParseError(ERR_EXPECTED, "no opening bracket after defined\n");
				}
				else
				{
					pr_file_p++;

					QCC_PR_SimpleGetToken();
					eval = !!QCC_PR_CheckCompConstDefined(pr_token);

					while (*pr_file_p == ' ' || *pr_file_p == '\t')
						pr_file_p++;
					if (*pr_file_p != ')')
						QCC_PR_ParseError(ERR_EXPECTED, "unclosed defined condition\n");
					pr_file_p++;
				}
			}
			else
			{
				if (!QCC_PR_SimpleGetToken())
					QCC_PR_ParseError(ERR_EXPECTED, "unexpected end-of-line\n");
				c = QCC_PR_CheckCompConstDefined(pr_token);
				if (!c)
					eval = atoi(pr_token);
				else
					eval = atoi(c->value);
			}
		}
		return eval;
	}

	eval = ParsePrecompilerIf(level-1);

	while (*pr_file_p && qcc_iswhite(*pr_file_p) && *pr_file_p != '\n')
	{
		pr_file_p++;
	}

	switch(level)
	{
	case PPI_LOGICAL:
		if (!strncmp(pr_file_p, "||", 2))
		{
			pr_file_p+=2;
			eval = ParsePrecompilerIf(level)||eval;
		}
		else if (!strncmp(pr_file_p, "&&", 2))
		{
			pr_file_p+=2;
			eval = ParsePrecompilerIf(level)&&eval;
		}
		break;
	case PPI_COMPARISON:
		if (!strncmp(pr_file_p, "<=", 2))
		{
			pr_file_p+=2;
			eval = eval <= ParsePrecompilerIf(level);
		}
		else if (!strncmp(pr_file_p, ">=", 2))
		{
			pr_file_p += 2;
			eval = eval >= ParsePrecompilerIf(level);
		}
		else if (!strncmp(pr_file_p, "<", 1))
		{
			pr_file_p += 1;
			eval = eval < ParsePrecompilerIf(level);
		}
		else if (!strncmp(pr_file_p, ">", 1))
		{
			pr_file_p += 1;
			eval = eval > ParsePrecompilerIf(level);
		}
		else if (!strncmp(pr_file_p, "!=", 2))
		{
			pr_file_p += 2;
			eval = eval != ParsePrecompilerIf(level);
		}
		else if (!strncmp(pr_file_p, "==", 2))
		{
			pr_file_p += 2;
			eval = eval == ParsePrecompilerIf(level);
		}
		break;
	}
	return eval;
}

//returns true if it was white/comments only. false if there was actual text that was skipped.
static void QCC_PR_SkipToEndOfLine(pbool errorifnonwhite)
{
	pbool handleccomments = true;
	while(*pr_file_p != '\n' && *pr_file_p != '\0')	//read on until the end of the line
	{
		if (*pr_file_p == '/' && pr_file_p[1] == '*' && handleccomments)
		{
			pr_file_p += 2;
			while(*pr_file_p)
			{
				if (*pr_file_p == '*' && pr_file_p[1] == '/')
				{
					pr_file_p+=2;
					break;
				}
				if (*pr_file_p == '\n')
					pr_source_line++;
				pr_file_p++;
			}
		}
		else if (*pr_file_p == '/' && pr_file_p[1] == '/' && handleccomments)
		{
			handleccomments = false;
			pr_file_p += 2;
			/*while(*pr_file_p)
			{
				if (*pr_file_p == '\n')
					break;
				pr_file_p++;
			}*/
		}
		else if (*pr_file_p == '\\' && pr_file_p[1] == '\r' && pr_file_p[2] == '\n')
		{	/*windows endings*/
			pr_file_p+=3;
			pr_source_line++;
		}
		else if (*pr_file_p == '\"' && handleccomments)
		{
			if (errorifnonwhite)
			{
				errorifnonwhite = false;
				QCC_PR_ParseWarning (ERR_UNKNOWNPUCTUATION, "unexpected tokens at end of line");
			}
			pr_file_p++;
			while (*pr_file_p)
			{
				if (*pr_file_p == '\n')
					break;	//this text is junk/ignored, so ignore the obvious error here.
				else if (*pr_file_p == '\"')
				{
					pr_file_p++;
					break;
				}
				else if (*pr_file_p == '\\' && pr_file_p[1] == '\"')
					pr_file_p+=2;	//don't trip on "\"/*"
				else if (*pr_file_p == '\\' && pr_file_p[1] == '\\')
					pr_file_p+=2;	//don't trip on "\\"//"foo
				else
					pr_file_p++;
				//any other \ should be part of the actual string, which we don't care about here
			}
		}
		else if (*pr_file_p == '\\' && pr_file_p[1] == '\n')
		{	/*linux endings*/
			pr_file_p+=2;

			pr_source_line++;
		}
		else
		{
			if (errorifnonwhite && handleccomments && !qcc_iswhite(*pr_file_p))
			{
				errorifnonwhite = false;
				QCC_PR_ParseWarning(ERR_UNKNOWNPUCTUATION, "unexpected tokens at end of line");
			}

			pr_file_p++;
		}
	}
}
/*
==============
QCC_PR_Precompiler
==============

Runs precompiler stage
*/
pbool QCC_PR_Precompiler(void)
{
	char msg[1024];
	int ifmode;
	int a;
	static int ifs = 0;
	int level;	//#if level
	pbool eval = false;

	if (*pr_file_p == '#')
	{
		char *directive;
		for (directive = pr_file_p+1; *directive; directive++)	//so #    define works
		{
			if (*directive == '\r' || *directive == '\n')
				QCC_PR_ParseError(ERR_UNKNOWNPUCTUATION, "Hanging # with no directive\n");
			if (*directive > ' ')
				break;
		}
		if (!strncmp(directive, "define", 6))
		{
			pr_file_p = directive;
			QCC_PR_PreProcessor_Define(false);
			QCC_PR_SkipToEndOfLine(true);
		}
		else if (!strncmp(directive, "append", 6))
		{
			pr_file_p = directive;
			QCC_PR_PreProcessor_Define(true);
			QCC_PR_SkipToEndOfLine(true);
		}
		else if (!strncmp(directive, "undef", 5))
		{
			pr_file_p = directive+5;
			while(qcc_iswhitesameline(*pr_file_p))
				pr_file_p++;

			QCC_PR_SimpleGetToken ();
			QCC_PR_UndefineName(pr_token);

	//		QCC_PR_ConditionCompilation();
			QCC_PR_SkipToEndOfLine(true);
		}
		else if (!strncmp(directive, "if", 2))
		{
			int originalline = pr_source_line;
 			pr_file_p = directive+2;
			if (!strncmp(pr_file_p, "def", 3))
			{
				ifmode = 0;
				pr_file_p+=3;
			}
			else if (!strncmp(pr_file_p, "ndef", 4))
			{
				ifmode = 1;
				pr_file_p+=4;
			}
			else
			{
				ifmode = 2;
				pr_file_p+=0;
				//QCC_PR_ParseError("bad \"#if\" type");
			}

			if (!qcc_iswhite(*pr_file_p))
			{
				pr_file_p = directive;
				QCC_PR_SimpleGetToken ();
				QCC_PR_ParseWarning(WARN_BADPRAGMA, "Unknown pragma \'%s\'", qcc_token);
			}
			else
			{
				if (ifmode == 2)
				{
					eval = ParsePrecompilerIf(PPI_TOPLEVEL);
				}
				else
				{
					QCC_PR_SimpleGetToken ();

		//			if (!STRCMP(pr_token, "COOP_MODE"))
		//				eval = false;
					if (QCC_PR_CheckCompConstDefined(pr_token))
						eval = true;

					if (ifmode == 1)
						eval = eval?false:true;
				}

				QCC_PR_SkipToEndOfLine(true);
				level = 1;

				if (eval)
					ifs+=1;
				else
				{
					while (1)
					{
						while(*pr_file_p && (*pr_file_p==' ' || *pr_file_p == '\t'))
							pr_file_p++;

						if (!*pr_file_p)
						{
							pr_source_line = originalline;
							QCC_PR_ParseError (ERR_NOENDIF, "#if with no endif");
						}

						if (*pr_file_p == '#')
						{
							pr_file_p++;
							while(*pr_file_p==' ' || *pr_file_p == '\t')
								pr_file_p++;
							if (!strncmp(pr_file_p, "endif", 5))
								level--;
							if (!strncmp(pr_file_p, "if", 2))
								level++;
							if (!strncmp(pr_file_p, "else", 4) && level == 1)
							{
								ifs+=1;
								pr_file_p+=4;
								QCC_PR_SkipToEndOfLine(true);
								break;
							}
						}

						QCC_PR_SkipToEndOfLine(false);

						if (level <= 0)
							break;
						pr_file_p++;	//next line
						pr_source_line++;
					}
				}
			}
		}
		else if (!strncmp(directive, "else", 4))
		{
			int originalline = pr_source_line;

			ifs -= 1;
			level = 1;

			pr_file_p = directive+4;
			QCC_PR_SkipToEndOfLine(true);
			while (1)
			{
				while(*pr_file_p && (*pr_file_p==' ' || *pr_file_p == '\t'))
					pr_file_p++;

				if (!*pr_file_p)
				{
					pr_source_line = originalline;
					QCC_PR_ParseError(ERR_NOENDIF, "#if with no endif");
				}

				if (*pr_file_p == '#')
				{
					pr_file_p++;
					while(*pr_file_p==' ' || *pr_file_p == '\t')
						pr_file_p++;

					if (!strncmp(pr_file_p, "endif", 5))
						level--;
					if (!strncmp(pr_file_p, "if", 2))
						level++;
					if (!strncmp(pr_file_p, "else", 4) && level == 1)
					{
						ifs+=1;
						pr_file_p+=4;
						QCC_PR_SkipToEndOfLine(true);
						break;
					}
				}

				QCC_PR_SkipToEndOfLine(false);
				if (level <= 0)
					break;
				pr_file_p++;	//go off the end
				pr_source_line++;
			}
		}
		else if (!strncmp(directive, "endif", 5))
		{
			pr_file_p = directive+5;
			QCC_PR_SkipToEndOfLine(true);
			if (ifs <= 0)
				QCC_PR_ParseError(ERR_NOPRECOMPILERIF, "unmatched #endif");
			else
				ifs-=1;
		}
		else if (!strncmp(directive, "eof", 3))
		{
			pr_file_p = NULL;
			return true;
		}
		else if (!strncmp(directive, "error", 5))
		{
			pr_file_p = directive+5;
			for (a = 0; a < sizeof(msg)-1 && pr_file_p[a] != '\t' && pr_file_p[a] != '\n' && pr_file_p[a] != '\0'; a++)
				msg[a] = pr_file_p[a];
			msg[a] = '\0';

			QCC_PR_SkipToEndOfLine(false);

			QCC_PR_ParseError(ERR_HASHERROR, "#Error: %s\n", msg);
		}
		else if (!strncmp(directive, "warning", 7))
		{
			pr_file_p = directive+7;
			for (a = 0; a < 1023 && pr_file_p[a] != '\r' && pr_file_p[a] != '\n' && pr_file_p[a] != '\0'; a++)
				msg[a] = pr_file_p[a];
			msg[a] = '\0';

			QCC_PR_SkipToEndOfLine(false);

			QCC_PR_ParseWarning(WARN_PRECOMPILERMESSAGE, "#warning: %s\n", msg);
		}
		else if (!strncmp(directive, "message", 7))
		{
			pr_file_p = directive+7;
			for (a = 0; a < sizeof(msg)-1 && pr_file_p[a] != '\r' && pr_file_p[a] != '\n' && pr_file_p[a] != '\0'; a++)
				msg[a] = pr_file_p[a];
			msg[a] = '\0';

			if (flag_msvcstyle)
				printf ("%s(%i) : #message: %s\n", strings + s_file, pr_source_line, msg);
			else
				printf ("%s:%i: #message: %s\n", strings + s_file, pr_source_line, msg);
			QCC_PR_SkipToEndOfLine(false);
		}
		else if (!strncmp(directive, "copyright", 9))
		{
			pr_file_p = directive+9;
			for (a = 0; a < sizeof(msg)-1 && qcc_iswhite(pr_file_p[a]) && pr_file_p[a] != '\0'; a++)
				msg[a] = pr_file_p[a];
			msg[a] = '\0';

			QCC_PR_SkipToEndOfLine(false);

			if (strlen(msg) >= sizeof(QCC_copyright))
				QCC_PR_ParseWarning(WARN_STRINGTOOLONG, "Copyright message is too long\n");
			QC_strlcpy(QCC_copyright, msg, sizeof(QCC_copyright)-1);
		}
		else if (!strncmp(directive, "pack", 4))
		{
			ifmode = 0;
			pr_file_p=directive+4;
			if (!strncmp(pr_file_p, "id", 2))
				pr_file_p+=3;
			else
			{
				ifmode = QCC_PR_LexInteger();
				if (ifmode == 0)
					ifmode = 1;
				pr_file_p++;
			}
			for (a = 0; a < sizeof(msg)-1 && qcc_iswhite(pr_file_p[a]) && pr_file_p[a] != '\0'; a++)
				msg[a] = pr_file_p[a];
			msg[a] = '\0';

			QCC_PR_SkipToEndOfLine(true);

			if (ifmode == 0)
				QCC_packid = atoi(msg);
			else if (ifmode <= 5)
				strcpy(QCC_Packname[ifmode-1], msg);
			else
				QCC_PR_ParseError(ERR_TOOMANYPACKFILES, "No more than 5 packs are allowed");
		}
		else if (!strncmp(directive, "forcecrc", 8))
		{
			pr_file_p=directive+8;

			ForcedCRC = QCC_PR_LexInteger();

			QCC_PR_SkipToEndOfLine(true);
		}
		else if (!strncmp(directive, "includelist", 11))
		{
			int defines=0;
			pr_file_p=directive+11;

			QCC_PR_SkipToEndOfLine(true);

			while(1)
			{
				QCC_PR_LexWhitespace(false);
				if (QCC_PR_CheckCompConst())
				{
					defines++;
					continue;
				}
				if (!QCC_PR_SimpleGetToken())
				{
					if (!*pr_file_p)
					{
						if (defines>0)
						{
							defines--;
							QCC_PR_UnInclude();
							continue;
						}
						QCC_Error(ERR_EOF, "eof in includelist");
					}
					else
					{
						pr_file_p++;
						pr_source_line++;
					}
					continue;
				}
				if (!strcmp(pr_token, "#endlist"))
				{
					QCC_PR_SkipToEndOfLine(true);
					break;
				}

				QCC_FindBestInclude(pr_token, compilingfile, qccmsourcedir, true);

				if (*pr_file_p == '\r')
					pr_file_p++;

//				QCC_PR_SkipToEndOfLine(true);
			}
		}
		else if (!strncmp(directive, "include", 7))
		{
			char sm;

			pr_file_p=directive+7;

			while(qcc_iswhitesameline(*pr_file_p))
				pr_file_p++;

			msg[0] = '\0';
			if (*pr_file_p == '\"')
				sm = '\"';
			else if (*pr_file_p == '<')
				sm = '>';
			else
			{
				QCC_PR_ParseError(0, "Not a string literal (on a #include)");
				sm = 0;
			}
			pr_file_p++;
			a=0;
			while(*pr_file_p != sm)
			{
				if (*pr_file_p == '\n')
				{
					QCC_PR_ParseError(0, "#include continued over line boundry\n");
					break;
				}
				msg[a++] = *pr_file_p;
				pr_file_p++;
			}
			msg[a] = 0;

			QCC_FindBestInclude(msg, compilingfile, qccmsourcedir, false);

			pr_file_p++;

			QCC_PR_SkipToEndOfLine(true);
		}
		else if (!strncmp(directive, "datafile", 8))
		{
			pr_file_p=directive+8;

			while(qcc_iswhitesameline(*pr_file_p))
				pr_file_p++;

			QCC_PR_LexString();
			printf("Including datafile: %s\n", pr_token);
			QCC_AddFile(pr_token);

			pr_file_p++;

			for (a = 0; a < sizeof(msg)-1 && pr_file_p[a] != '\n' && pr_file_p[a] != '\0'; a++)
				msg[a] = pr_file_p[a];

			msg[a-1] = '\0';

			while(*pr_file_p != '\n' && *pr_file_p != '\0')	//read on until the end of the line
			{
				pr_file_p++;
			}
		}
		else if (!strncmp(directive, "output", 6))
		{
			extern char		destfile[1024];
			pr_file_p=directive+6;

			while(qcc_iswhitesameline(*pr_file_p))
				pr_file_p++;

			QCC_PR_LexString();
			strcpy(destfile, pr_token);
			printf("Outputfile: %s\n", destfile);

			pr_file_p++;

			for (a = 0; a < sizeof(msg)-1 && pr_file_p[a] != '\n' && pr_file_p[a] != '\0'; a++)
				msg[a] = pr_file_p[a];

			msg[a-1] = '\0';

			while(*pr_file_p != '\n' && *pr_file_p != '\0')	//read on until the end of the line
			{
				pr_file_p++;
			}
		}
		else if (!strncmp(directive, "pragma", 6))
		{
			pr_file_p=directive+6;
			while(qcc_iswhitesameline(*pr_file_p))
				pr_file_p++;

			qcc_token[0] = '\0';
			for(a = 0; *pr_file_p != '\n' && *pr_file_p != '\0'; pr_file_p++)	//read on until the end of the line
			{
				if ((*pr_file_p == ' ' || *pr_file_p == '\t'|| *pr_file_p == '(') && !*qcc_token)
				{
					msg[a] = '\0';
					strcpy(qcc_token, msg);
					a=0;
					continue;
				}
				msg[a++] = *pr_file_p;
			}

			msg[a] = '\0';
			{
				char *end;
				for (end = msg + a-1; end>=msg && qcc_iswhite(*end); end--)
					*end = '\0';
			}

			if (!*qcc_token)
			{
				strcpy(qcc_token, msg);
				msg[0] = '\0';
			}

			{
				char *end;
				for (end = msg + a-1; end>=msg && qcc_iswhite(*end); end--)
					*end = '\0';
			}

			if (!QC_strcasecmp(qcc_token, "DONT_COMPILE_THIS_FILE"))
			{
				while (*pr_file_p)
				{
					while(*pr_file_p != '\n' && *pr_file_p != '\0')	//read on until the end of the line
						pr_file_p++;

					if (*pr_file_p == '\n')
					{
						pr_file_p++;
						QCC_PR_NewLine(false);
					}
				}
			}
			else if (!QC_strcasecmp(qcc_token, "COPYRIGHT"))
			{
				if (strlen(msg) >= sizeof(QCC_copyright))
					QCC_PR_ParseWarning(WARN_STRINGTOOLONG, "Copyright message is too long\n");
				QC_strlcpy(QCC_copyright, msg, sizeof(QCC_copyright)-1);
			}
			else if (!QC_strcasecmp(qcc_token, "compress"))
			{
				extern pbool compressoutput;
				compressoutput = atoi(msg);
			}
			else if (!QC_strcasecmp(qcc_token, "forcecrc"))
			{
				ForcedCRC = atoi(msg);
			}
			else if (!QC_strcasecmp(qcc_token, "noref"))
			{
				defaultnoref = !!atoi(msg);
			}
			else if (!QC_strcasecmp(qcc_token, "defaultstatic"))
			{
				defaultstatic = !!atoi(msg);
			}
			else if (!QC_strcasecmp(qcc_token, "autoproto"))
			{
				if (!autoprototyped)
				{
					if (numpr_globals != RESERVED_OFS)
						QCC_PR_ParseWarning(WARN_BADPRAGMA, "#pragma autoproto must appear before any definitions");
					else
						autoprototype = *msg?!!atoi(msg):true;
				}
			}
			else if (!QC_strcasecmp(qcc_token, "wrasm"))
			{
				pbool on = atoi(msg);

				if (asmfile && !on)
				{
					fclose(asmfile);
					asmfile = NULL;
				}
				if (!asmfile && on)
				{
					if (asmfilebegun)
						asmfile = fopen("qc.asm", "ab");
					else
						asmfile = fopen("qc.asm", "wb");
					if (asmfile)
						asmfilebegun = true;
				}
			}
			else if (!QC_strcasecmp(qcc_token, "optimise") || !QC_strcasecmp(qcc_token, "optimize"))	//bloomin' americans.
			{
				int o;
				extern pbool qcc_nopragmaoptimise;
				if (pr_scope)
					QCC_PR_ParseWarning(WARN_BADPRAGMA, "pragma %s: unable to change optimisation options mid-function", qcc_token);
				else if (*msg >= '0' && *msg <= '3')
				{
					int lev = atoi(msg);
					pbool state;
					int once = false;
					for (o = 0; optimisations[o].enabled; o++)
					{
						state = optimisations[o].optimisationlevel <= lev;
						if (qcc_nopragmaoptimise && *optimisations[o].enabled != state)
						{
							if (!once++)
								QCC_PR_ParseWarning(WARN_BADPRAGMA, "pragma %s %s: overriden by commandline", qcc_token, msg);
						}
						else
							*optimisations[o].enabled = state;
					}
				}
				else if (!strnicmp(msg, "addon", 5) || !strnicmp(msg, "mutator", 7))
				{
					int lev = 2;
					pbool state = false;
					for (o = 0; optimisations[o].enabled; o++)
					{
						if (optimisations[o].optimisationlevel > lev)
						{
							if (qcc_nopragmaoptimise && *optimisations[o].enabled != state)
								QCC_PR_ParseWarning(WARN_IGNORECOMMANDLINE, "pragma %s %s: disabling %s", qcc_token, msg, optimisations[o].fullname);
							*optimisations[o].enabled = state;
						}
					}
				}
				else
				{
					char *opt = msg;
					pbool state = true;
					if (!strnicmp(msg, "no-", 3))
					{
						state = false;
						opt += 3;
					}
					for (o = 0; optimisations[o].enabled; o++)
						if ((*optimisations[o].abbrev && !stricmp(opt, optimisations[o].abbrev)) || !stricmp(opt, optimisations[o].fullname))
						{
							if (qcc_nopragmaoptimise && *optimisations[o].enabled != state)
								QCC_PR_ParseWarning(WARN_BADPRAGMA, "pragma %s %s: overriden by commandline", qcc_token, optimisations[o].fullname);
							else
								*optimisations[o].enabled = state;
							break;
						}
					if (!optimisations[o].enabled)
						QCC_PR_ParseWarning(WARN_BADPRAGMA, "pragma %s: %s unsupported", qcc_token, opt);
				}
			}
			else if (!QC_strcasecmp(qcc_token, "sourcefile"))
			{
	#define MAXSOURCEFILESLIST 8
	extern char sourcefileslist[MAXSOURCEFILESLIST][1024];
	//extern int currentsourcefile; // warning: unused variable �currentsourcefile�
	extern int numsourcefiles;

				int i;

				QCC_COM_Parse(msg);

				for (i = 0; i < numsourcefiles; i++)
				{
					if (!strcmp(sourcefileslist[i], qcc_token))
						break;
				}
				if (i == numsourcefiles && numsourcefiles < MAXSOURCEFILESLIST)
					strcpy(sourcefileslist[numsourcefiles++], qcc_token);
			}
			else if (!QC_strcasecmp(qcc_token, "TARGET"))
			{
				int newtype = qcc_targetformat;
				QCC_COM_Parse(msg);
				if (!QC_strcasecmp(qcc_token, "H2") || !QC_strcasecmp(qcc_token, "HEXEN2"))
					newtype = QCF_HEXEN2;
				else if (!QC_strcasecmp(qcc_token, "KK7"))
					newtype = QCF_KK7;
				else if (!QC_strcasecmp(qcc_token, "DP") || !QC_strcasecmp(qcc_token, "DARKPLACES"))
					newtype = QCF_DARKPLACES;
				else if (!QC_strcasecmp(qcc_token, "FTEDEBUG"))
					newtype = QCF_FTEDEBUG;
				else if (!QC_strcasecmp(qcc_token, "FTE"))
					newtype = QCF_FTE;
				else if (!QC_strcasecmp(qcc_token, "FTEH2"))
					newtype = QCF_FTEH2;
				else if (!QC_strcasecmp(qcc_token, "STANDARD") || !QC_strcasecmp(qcc_token, "ID"))
					newtype = QCF_STANDARD;
				else if (!QC_strcasecmp(qcc_token, "DEBUG"))
					newtype = QCF_FTEDEBUG;
				else if (!QC_strcasecmp(qcc_token, "QTEST"))
					newtype = QCF_QTEST;
				else
					QCC_PR_ParseWarning(WARN_BADTARGET, "Unknown target \'%s\'. Ignored.\nValid targets are: ID, HEXEN2, FTE, FTEH2, KK7, DP(patched)", qcc_token);

				if (numstatements > 1)
				{
					if ((qcc_targetformat == QCF_HEXEN2 || qcc_targetformat == QCF_FTEH2) && (newtype != QCF_HEXEN2 && newtype != QCF_FTEH2))
						QCC_PR_ParseWarning(WARN_BADTARGET, "Cannot switch from hexen2 target \'%s\' after the first statement. Ignored.", msg);
					if ((newtype == QCF_HEXEN2 || newtype == QCF_FTEH2) && (qcc_targetformat != QCF_HEXEN2 && qcc_targetformat != QCF_FTEH2))
						QCC_PR_ParseWarning(WARN_BADTARGET, "Cannot switch to hexen2 target \'%s\' after the first statement. Ignored.", msg);
				}

				qcc_targetformat = newtype;
			}
			else if (!QC_strcasecmp(qcc_token, "PROGS_SRC"))
			{	//doesn't make sence, but silenced if you are switching between using a certain precompiler app used with CuTF.
			}
			else if (!QC_strcasecmp(qcc_token, "PROGS_DAT"))
			{	//doesn't make sence, but silenced if you are switching between using a certain precompiler app used with CuTF.
				extern char		destfile[1024];
				char olddest[1024];
#ifndef QCCONLY
				extern char qccmfilename[1024];
				int p;
				char *s, *s2;
#endif
				Q_strlcpy(olddest, destfile, sizeof(olddest));
				QCC_COM_Parse(msg);

#ifndef QCCONLY
	p=0;
	s2 = qcc_token;
	if (!strncmp(s2, "./", 2))
		s2+=2;
	else
	{
		while(!strncmp(s2, "../", 3))
		{
			s2+=3;
			p++;
		}
	}
	strcpy(qccmfilename, qccmsourcedir);
	for (s=qccmfilename+strlen(qccmfilename);p && s>=qccmfilename; s--)
	{
		if (*s == '/' || *s == '\\')
		{
			*(s+1) = '\0';
			p--;
		}
	}
	QC_snprintfz(destfile, sizeof(destfile), "%s", s2);

	while (p>0)
	{
		memmove(destfile+3, destfile, strlen(destfile)+1);
		destfile[0] = '.';
		destfile[1] = '.';
		destfile[2] = '/';
		p--;
	}
#else

				strcpy(destfile, qcc_token);
#endif
				if (strcmp(destfile, olddest))
					printf("Outputfile: %s\n", destfile);
			}
			else if (!QC_strcasecmp(qcc_token, "keyword") || !QC_strcasecmp(qcc_token, "flag"))
			{
				char *s;
				int st;
				s = QCC_COM_Parse(msg);
				if (!QC_strcasecmp(qcc_token, "enable") || !QC_strcasecmp(qcc_token, "on"))
					st = 1;
				else if (!QC_strcasecmp(qcc_token, "disable") || !QC_strcasecmp(qcc_token, "off"))
					st = 0;
				else
				{
					QCC_PR_ParseWarning(WARN_BADPRAGMA, "compiler flag state not recognised");
					st = -1;
				}
				if (st < 0)
					QCC_PR_ParseWarning(WARN_BADPRAGMA, "warning id not recognised");
				else
				{
					int f;
					s = QCC_COM_Parse(s);

					for (f = 0; compiler_flag[f].enabled; f++)
					{
						if (!QC_strcasecmp(compiler_flag[f].abbrev, qcc_token))
						{
							if (compiler_flag[f].flags & FLAG_MIDCOMPILE)
								*compiler_flag[f].enabled = st;
							else
								QCC_PR_ParseWarning(WARN_BADPRAGMA, "Cannot enable/disable keyword/flag via a pragma");
							break;
						}
					}
					if (!compiler_flag[f].enabled)
						QCC_PR_ParseWarning(WARN_BADPRAGMA, "keyword/flag %s not recognised", qcc_token);

				}
			}
			else if (!QC_strcasecmp(qcc_token, "warning"))
			{
				int st;
				char *s;
				s = QCC_COM_Parse(msg);
				if (!stricmp(qcc_token, "enable") || !stricmp(qcc_token, "on"))
					st = WA_WARN;
				else if (!stricmp(qcc_token, "disable") || !stricmp(qcc_token, "off") || !stricmp(qcc_token, "ignore"))
					st = WA_IGNORE;
				else if (!stricmp(qcc_token, "error"))
					st = WA_ERROR;
				else if (!stricmp(qcc_token, "toggle"))
					st = 3;
				else
				{
					QCC_PR_ParseWarning(WARN_BADPRAGMA, "warning state not recognised");
					st = -1;
				}
				if (st>=0)
				{
					int wn;
					s = QCC_COM_Parse(s);
					wn = QCC_WarningForName(qcc_token);
					if (wn < 0)
						QCC_PR_ParseWarning(WARN_BADPRAGMA, "warning id not recognised");
					else
					{
						if (st == 3)	//toggle
							qccwarningaction[wn] = !!qccwarningaction[wn];
						else
							qccwarningaction[wn] = st;
					}
				}
			}
			else
				QCC_PR_ParseWarning(WARN_BADPRAGMA, "Unknown pragma \'%s\'", qcc_token);
		}
		return true;
	}

	return false;
}

/*
==============
PR_NewLine

Call at start of file and when *pr_file_p == '\n'
==============
*/
void QCC_PR_NewLine (pbool incomment)
{
	pr_source_line++;
	pr_line_start = pr_file_p;
	while(*pr_file_p==' ' || *pr_file_p == '\t')
		pr_file_p++;
	if (incomment)	//no constants if in a comment.
	{
	}
	else if (QCC_PR_Precompiler())
	{
	}

//	if (pr_dumpasm)
//		PR_PrintNextLine ();
}

/*
==============
PR_LexString

Parses a quoted string
==============
*/
#if 0
void QCC_PR_LexString (void)
{
	int		c;
	int		len;
	char tmpbuf[2048];

	char *text;
	char *oldf;
	int oldline;

	bool fromfile = true;

	len = 0;

	text = pr_file_p;
	do
	{
		QCC_COM_Parse(text);
//		print("Next token is \"%s\"\n", com_token);
		if (*text == '\"')
		{
			text++;
			if (fromfile) pr_file_p++;
		}
	do
	{
		c = *text++;
		if (fromfile) pr_file_p++;
		if (!c)
			QCC_PR_ParseError ("EOF inside quote");
		if (c=='\n')
			QCC_PR_ParseError ("newline inside quote");
		if (c=='\\')
		{	// escape char
			c = *text++;
			if (fromfile) pr_file_p++;
			if (!c)
				QCC_PR_ParseError ("EOF inside quote");
			if (c == 'n')
				c = '\n';
			else if (c == '"')
				c = '"';
			else if (c == '\\')
				c = '\\';
			else
				QCC_PR_ParseError ("Unknown escape char");
		}
		else if (c=='\"')
		{
			if (fromfile) pr_file_p++;
			break;
		}
		tmpbuf[len] = c;
		len++;
	} while (1);
		tmpbuf[len] = 0;
//		if (fromfile) pr_file_p++;

		pr_immediate_type=NULL;
		oldline=pr_source_line;
		oldf=pr_file_p;
		QCC_PR_Lex();
		if (pr_immediate_type == &type_string)
		{
//			print("Appending \"%s\" to \"%s\"\n", pr_immediate_string, tmpbuf);
			strcat(tmpbuf, pr_immediate_string);
			len+=strlen(pr_immediate_string);
		}
		else
		{
			pr_source_line = oldline;
			pr_file_p = oldf-1;
			QCC_PR_LexWhitespace();
			if (*pr_file_p != '\"')	//annother string
				break;
		}

		QCC_PR_LexWhitespace();
		text = pr_file_p;

	} while (1);

	strcpy(pr_token, tmpbuf);
	pr_token_type = tt_immediate;
	pr_immediate_type = &type_string;
	strcpy (pr_immediate_string, pr_token);

//	print("Found \"%s\"\n", pr_immediate_string);
}
#else
void QCC_PR_LexString (void)
{
	int		c;
	int		len = 0;
	char	*end, *cnst;
	int		raw;
	char	rawdelim[64];
	unsigned int		code;

	int texttype;
	pbool first = true;

	for(;;)
	{
		raw = 0;
		texttype = 0;

		QCC_PR_LexWhitespace(false);

		if (*pr_file_p == 'R' && pr_file_p[1] == '\"')
		{
			/*R"delim(fo
			o)delim" -> "fo\no"
			the []
			*/
			raw = 1;
			pr_file_p+=2;

			while (1)
			{
				c = *pr_file_p++;
				if (c == '(')
				{
					rawdelim[0] = ')';
					break;
				}
				if (!c || raw >= sizeof(rawdelim)-1)
					QCC_PR_ParseError (ERR_EOF, "EOF while parsing raw string delimiter. Expected: R\"delim(string)delim\"");
				rawdelim[raw++] = c;
			}
			rawdelim[raw++] = '\"';

			//these two conditions are generally part of the C preprocessor.
			if (!strncmp(pr_file_p, "\\\r\n", 3))
			{	//dos format
				pr_file_p += 3;
				pr_source_line++;
			}
			else if (!strncmp(pr_file_p, "\\\r", 2) || !strncmp(pr_file_p, "\\\n", 2))
			{	//mac + unix format
				pr_file_p += 2;
				pr_source_line++;
			}
		}
		else if (*pr_file_p == '\"')
			pr_file_p++;
		else if (first)
			QCC_PR_ParseError(ERR_BADCHARACTERCODE, "Expected string constant");
		else
			break;
		first = false;

		for(;;)
		{
			c = *pr_file_p++;
			if (!c)
				QCC_PR_ParseError (ERR_EOF, "EOF inside quote");

			if (!qccwarningaction[WARN_NOTUTF8] && c < 0 && utf8_check(&pr_token[c-1], &code))
			{
				//convert 0xe000 private-use area to quake's charset (if they don't have the utf-8 warning enabled)
				//note: this may have a small false-positive risk.
				if (code >= 0xe000 && code <= 0xe0ff)
					pr_file_p += utf8_check(&pr_token[c-1], &code)-1;
			}

/*			//these two conditions are generally part of the C preprocessor.
			if (c == '\\' && *pr_file_p == '\r' && pr_file_p[1] == '\n')
			{	//dos format
				pr_file_p += 2;
				pr_source_line++;
				continue;
			}
			if (c == '\\' && (*pr_file_p == '\r' || pr_file_p[1] == '\n'))
			{	//mac + unix format
				pr_file_p += 1;
				pr_source_line++;
				continue;
			}
*/
			if (raw)
			{
				//raw strings contain very little parsing. just delimiter and initial \NL support.
				if (c == rawdelim[0] && !strncmp(pr_file_p, rawdelim+1, raw-1))
				{
					pr_file_p += raw-1;
					break;
				}

				//make sure line numbers are correct
				if (c == '\r' && *pr_file_p != '\n')
					pr_source_line++;	//mac
				if (c == '\n')	//dos/unix
					pr_source_line++;
			}
			else
			{
				if (c=='\n')
					QCC_PR_ParseError (ERR_INVALIDSTRINGIMMEDIATE, "newline inside quote");
				if (c=='\\')
				{	// escape char
					c = *pr_file_p++;
					if (!c)
						QCC_PR_ParseError (ERR_EOF, "EOF inside quote");
					if (c == 'n')
						c = '\n';
					else if (c == 'r')
						c = '\r';
					else if (c == '"')
						c = '"';
					else if (c == 't')
						c = '\t';	//tab
					else if (c == 'a')
						c = '\a';	//bell
					else if (c == 'v')
						c = '\v';	//vertical tab
					else if (c == 'f')
						c = '\f';	//form feed
					else if (c == 's' || c == 'b')
					{
						texttype ^= 128;
						continue;
					}
					//else if (c == 'b')
					//	c = '\b';
					else if (c == '[')
						c = 16;	//quake specific
					else if (c == ']')
						c = 17;	//quake specific
					else if (c == '{')
					{
						int d;
						c = 0;
						while ((d = *pr_file_p++) != '}')
						{
							c = c * 10 + d - '0';
							if (d < '0' || d > '9' || c > 255)
								QCC_PR_ParseError(ERR_BADCHARACTERCODE, "Bad character code");
						}
					}
					else if (c == '.')
						c = 0x1c | texttype;
					else if (c == '<')
						c = 29;
					else if (c == '-')
						c = 30;
					else if (c == '>')
						c = 31;
					else if (c == 'u' || c == 'U')
					{
						//lower case u specifies exactly 4 nibbles.
						//upper case U specifies variable length. terminate with a double-doublequote pair, or some other non-hex char.
						int count = 0;
						unsigned long d;
						unsigned long unicode;
						unicode = 0;
						for(;;)
						{
							d = (unsigned char)*pr_file_p;
							if (d >= '0' && d <= '9')
								unicode = (unicode*16) + (d - '0');
							else if (d >= 'A' && d <= 'F')
								unicode = (unicode*16) + (d - 'A') + 10;
							else if (d >= 'a' && d <= 'f')
								unicode = (unicode*16) + (d - 'a') + 10;
							else
								break;
							count++;
							pr_file_p++;
						}
						if (!count || ((c=='u')?(count!=4):(count>8)) || unicode > 0x10FFFFu)	//RFC 3629 imposes the same limit as UTF-16 surrogate pairs.
							QCC_PR_ParseWarning(ERR_BADCHARACTERCODE, "Bad unicode character code");

						//figure out the count of bytes required to encode this char
						count = 1;
						d = 0x7f;
						while (unicode > d)
						{
							count++;
							d = (d<<5) | 0x1f;
						}

						//error if needed
						if (len+count >= sizeof(pr_token))
							QCC_Error(ERR_INVALIDSTRINGIMMEDIATE, "String length exceeds %i", sizeof(pr_token)-1);

						//output it.
						if (count == 1)
							pr_token[len++] = (unsigned char)(c&0x7f);
						else
						{
							c = count*6;
							pr_token[len++] = (unsigned char)((unicode>>c)&(0x0000007f>>count)) | (0xffffff00 >> count);
							do
							{
								c = c-6;
								pr_token[len++] = (unsigned char)((unicode>>c)&0x3f) | 0x80;
							}
							while(c);
						}
						continue;
					}
					else if (c == 'x' || c == 'X')
					{
						int d;
						c = 0;

						d = (unsigned char)*pr_file_p++;
						if (d >= '0' && d <= '9')
							c += d - '0';
						else if (d >= 'A' && d <= 'F')
							c += d - 'A' + 10;
						else if (d >= 'a' && d <= 'f')
							c += d - 'a' + 10;
						else
							QCC_PR_ParseError(ERR_BADCHARACTERCODE, "Bad character code");

						c *= 16;

						d = (unsigned char)*pr_file_p++;
						if (d >= '0' && d <= '9')
							c += d - '0';
						else if (d >= 'A' && d <= 'F')
							c += d - 'A' + 10;
						else if (d >= 'a' && d <= 'f')
							c += d - 'a' + 10;
						else
							QCC_PR_ParseError(ERR_BADCHARACTERCODE, "Bad character code");
					}
					else if (c == '\\')
						c = '\\';
					else if (c == '\'')
						c = '\'';
					else if (c >= '0' && c <= '9')	//WARNING: This is not octal, but uses 'yellow' numbers instead (as on hud).
						c = 18 + c - '0';
					else if (c == '\r')
					{	//sigh
						c = *pr_file_p++;
						if (c != '\n')
							QCC_PR_ParseWarning(WARN_HANGINGSLASHR, "Hanging \\\\\r");
						pr_source_line++;
					}
					else if (c == '\n')
					{	//sigh
						pr_source_line++;
					}
					else
						QCC_PR_ParseError (ERR_INVALIDSTRINGIMMEDIATE, "Unknown escape char %c", c);
				}
				else if (c=='\"')
				{
					break;
				}
				else if (c == '#')
				{
					for (end = pr_file_p; ; end++)
					{
						if (qcc_iswhite(*end))
							break;

						if (*end == ')'
							||	*end == '('
							||	*end == '+'
							||	*end == '-'
							||	*end == '*'
							||	*end == '/'
							||	*end == '\\'
							||	*end == '|'
							||	*end == '&'
							||	*end == '='
							||	*end == '^'
							||	*end == '~'
							||	*end == '['
							||	*end == ']'
							||	*end == '\"'
							||	*end == '{'
							||	*end == '}'
							||	*end == ';'
							||	*end == ':'
							||	*end == ','
							||	*end == '.'
							||	*end == '#')
								break;
					}

					c = *end;
					*end = '\0';
					cnst = QCC_PR_CheckCompConstString(pr_file_p);
					if (cnst==pr_file_p)
						cnst=NULL;
					*end = c;
					c = '#';	//undo
					if (cnst)
					{
						QCC_PR_ParseWarning(WARN_MACROINSTRING, "Macro expansion in string");

						if (len+strlen(cnst) >= sizeof(pr_token)-1)
							QCC_Error(ERR_INVALIDSTRINGIMMEDIATE, "String length exceeds %i", sizeof(pr_token)-1);

						strcpy(pr_token+len, cnst);
						len+=strlen(cnst);
						pr_file_p = end;
						continue;
					}
				}
				else if (c == 0x7C && flag_acc)	//reacc support... reacc is strange.
					c = '\n';
				else
					c |= texttype;
			}

			if (len >= sizeof(pr_token)-1)
				QCC_Error(ERR_INVALIDSTRINGIMMEDIATE, "String length exceeds %i", sizeof(pr_token)-1);
			pr_token[len] = c;
			len++;
		}
	}

	if (len > sizeof(pr_immediate_string)-1)
		QCC_Error(ERR_INVALIDSTRINGIMMEDIATE, "String length exceeds %i", sizeof(pr_immediate_string)-1);

	pr_token[len] = 0;
	pr_token_type = tt_immediate;
	pr_immediate_type = type_string;
	strcpy (pr_immediate_string, pr_token);

	if (qccwarningaction[WARN_NOTUTF8])
	{
		for (c = 0; pr_token[c]; c++)
		{
			len = utf8_check(&pr_token[c], &code);
			if (!len)
			{
				QCC_PR_ParseWarning(WARN_NOTUTF8, "String constant is not valid utf-8");
				break;
			}
		}
	}
}
#endif

/*
==============
PR_LexNumber
==============
*/
int QCC_PR_LexInteger (void)
{
	int		c;
	int		len;

	len = 0;
	c = *pr_file_p;
	if (pr_file_p[0] == '0' && pr_file_p[1] == 'x')
	{
		pr_token[0] = '0';
		pr_token[1] = 'x';
		len = 2;
		c = *(pr_file_p+=2);
	}
	do
	{
		pr_token[len] = c;
		len++;
		pr_file_p++;
		c = *pr_file_p;
	} while ((c >= '0' && c<= '9') || (c == '.'&&pr_file_p[1]!='.') || (c>='a' && c <= 'f'));
	pr_token[len] = 0;
	return atoi (pr_token);
}

void QCC_PR_LexNumber (void)
{
	int tokenlen = 0;
	int num=0;
	int base=0;
	int c;
	int sign=1;
	if (*pr_file_p == '-')
	{
		sign=-1;
		pr_file_p++;

		pr_token[tokenlen++] = '-';
	}
	if (pr_file_p[0] == '0' && pr_file_p[1] == 'x')
	{
		pr_file_p+=2;
		base = 16;

		pr_token[tokenlen++] = '0';
		pr_token[tokenlen++] = 'x';
	}

	pr_immediate_type = NULL;
	//assume base 10 if not stated
	if (!base)
		base = 10;

	while((c = *pr_file_p))
	{
		if (c >= '0' && c <= '9')
		{
			pr_token[tokenlen++] = c;
			num*=base;
			num += c-'0';
		}
		else if (c >= 'a' && c <= 'f' && base > 10)
		{
			pr_token[tokenlen++] = c;
			num*=base;
			num += c -'a'+10;
		}
		else if (c >= 'A' && c <= 'F' && base > 10)
		{
			pr_token[tokenlen++] = c;
			num*=base;
			num += c -'A'+10;
		}
		else if (c == '.' && pr_file_p[1]!='.')
		{
			pr_token[tokenlen++] = c;
			pr_file_p++;
			pr_immediate_type = type_float;
			while(1)
			{
				c = *pr_file_p;
				if (c >= '0' && c <= '9')
				{
					pr_token[tokenlen++] = c;
				}
				else if (c == 'f')
				{
					pr_file_p++;
					break;
				}
				else
				{
					break;
				}
				pr_file_p++;
			}
			pr_token[tokenlen++] = 0;
			pr_immediate._float = (float)atof(pr_token);
			return;
		}
		else if (c == 'f')
		{
			pr_token[tokenlen++] = c;
			pr_token[tokenlen++] = 0;
			pr_file_p++;
			pr_immediate_type = type_float;
			pr_immediate._float = num*sign;
			return;
		}
		else if (c == 'i')
		{
			pr_token[tokenlen++] = c;
			pr_token[tokenlen++] = 0;
			pr_file_p++;
			pr_immediate_type = type_integer;
			pr_immediate._int = num*sign;
			return;
		}
		else
			break;
		pr_file_p++;
	}
	pr_token[tokenlen++] = 0;

	if (!pr_immediate_type)
	{
		if (flag_assume_integer)
			pr_immediate_type = type_integer;
		else
			pr_immediate_type = type_float;
	}

	if (pr_immediate_type == type_integer)
	{
		pr_immediate_type = type_integer;
		pr_immediate._int = num*sign;
	}
	else
	{
		pr_immediate_type = type_float;
		// at this point, we know there's no . in it, so the NaN bug shouldn't happen
		// and we cannot use atof on tokens like 0xabc, so use num*sign, it SHOULD be safe
		//pr_immediate._float = atof(pr_token);
		pr_immediate._float = (float)(num*sign);
	}
}


float QCC_PR_LexFloat (void)
{
	int		c;
	int		len;

	len = 0;
	c = *pr_file_p;
	do
	{
		pr_token[len] = c;
		len++;
		pr_file_p++;
		c = *pr_file_p;
	} while ((c >= '0' && c<= '9') || (c == '.'&&pr_file_p[1]!='.'));	//only allow a . if the next isn't too...
	if (*pr_file_p == 'f')
		pr_file_p++;
	pr_token[len] = 0;
	return (float)atof (pr_token);
}

/*
==============
PR_LexVector

Parses a single quoted vector
==============
*/
void QCC_PR_LexVector (void)
{
	int		i;

	pr_file_p++;

	if (*pr_file_p == '\\')
	{//extended character constant
		pr_token_type = tt_immediate;
		pr_immediate_type = type_float;
		pr_file_p++;
		switch(*pr_file_p)
		{
		case 'n':
			pr_immediate._float = '\n';
			break;
		case 'r':
			pr_immediate._float = '\r';
			break;
		case 't':
			pr_immediate._float = '\t';
			break;
		case '\'':
			pr_immediate._float = '\'';
			break;
		case '\"':
			pr_immediate._float = '\"';
			break;
		case '\\':
			pr_immediate._float = '\\';
			break;
		default:
			QCC_PR_ParseError (ERR_INVALIDVECTORIMMEDIATE, "Bad character constant");
		}
		pr_file_p++;
		if (*pr_file_p != '\'')
			QCC_PR_ParseError (ERR_INVALIDVECTORIMMEDIATE, "Bad character constant");
		pr_file_p++;
		return;
	}
	if (pr_file_p[1] == '\'')
	{//character constant
		pr_token_type = tt_immediate;
		pr_immediate_type = type_float;
		pr_immediate._float = pr_file_p[0];
		pr_file_p+=2;
		return;
	}
	pr_token_type = tt_immediate;
	pr_immediate_type = type_vector;
	QCC_PR_LexWhitespace (false);
	for (i=0 ; i<3 ; i++)
	{
		pr_immediate.vector[i] = QCC_PR_LexFloat ();
		QCC_PR_LexWhitespace (false);

		if (*pr_file_p == '\'' && i == 1)
		{
			if (i < 2)
				QCC_PR_ParseWarning (WARN_FTE_SPECIFIC, "Bad vector");

			for (i++ ; i<3 ; i++)
				pr_immediate.vector[i] = 0;
			break;
		}
	}
	if (*pr_file_p != '\'')
		QCC_PR_ParseError (ERR_INVALIDVECTORIMMEDIATE, "Bad vector");
	pr_file_p++;
}

/*
==============
PR_LexName

Parses an identifier
==============
*/
void QCC_PR_LexName (void)
{
	unsigned int		c;
	int		len;

	len = 0;
	do
	{
		int b = utf8_check(pr_file_p, &c);
		if (!b)
		{
			unsigned char lead = *pr_file_p++;
			char *o;
			while(*pr_file_p && !utf8_check(pr_file_p, &c))
				pr_file_p++;
			o = pr_file_p;
			while (qcc_iswhite(*pr_file_p))
			{
				if (*pr_file_p == '\n')
					break;
				pr_file_p++;
			}
			if (*pr_file_p == '\n')
				QCC_PR_ParseError(ERR_NOTANAME, "Invalid UTF-8 code sequence at end of line. Lead byte was %#2x", lead);
			else
			{
				len = 0;
				while (*pr_file_p && !qcc_iswhite(*pr_file_p))
					pr_token[len++] = *pr_file_p++;
				pr_token[len++] = 0;
				pr_file_p = o;
				QCC_PR_ParseError(ERR_NOTANAME, "Invalid UTF-8 code sequence before %s. Lead byte was %#2x", pr_token, lead);
			}
			return;
		}
		while(b-->0)
		{
			pr_token[len] = *pr_file_p++;
			len++;
		}
		c = *pr_file_p;
	} while ( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'
	|| (c >= '0' && c <= '9') || c & 0x80);

	pr_token[len] = 0;
	pr_token_type = tt_name;
}

/*
==============
PR_LexPunctuation
==============
*/
void QCC_PR_LexPunctuation (void)
{
	int		i;
	int		len;
	char	*p;

	pr_token_type = tt_punct;

	for (i=0 ; (p = pr_punctuation[i]) != NULL ; i++)
	{
		len = strlen(p);
		if (!strncmp(p, pr_file_p, len) )
		{
			strcpy (pr_token, pr_punctuationremap[i]);
			if (p[0] == '{')
				pr_bracelevel++;
			else if (p[0] == '}')
				pr_bracelevel--;
			pr_file_p += len;
			return;
		}
	}

	if ((unsigned char)*pr_file_p == (unsigned char)0xa0)
		QCC_PR_ParseWarning (ERR_UNKNOWNPUCTUATION, "Unknown punctuation: '\\x%x' - non-breaking space", (unsigned char)*pr_file_p);
	else
		QCC_PR_ParseWarning (ERR_UNKNOWNPUCTUATION, "Unknown punctuation: '\\x%x'", *pr_file_p);
	pr_file_p++;

	QCC_PR_Lex();
}


/*
==============
PR_LexWhitespace
==============
*/
void QCC_PR_LexWhitespace (pbool inhibitpreprocessor)
{
	int		c;

	while (1)
	{
	// skip whitespace
		while ((c = *pr_file_p) && qcc_iswhite(c))
		{
			if (c=='\n')
			{
				pr_file_p++;
				if (!inhibitpreprocessor)
					QCC_PR_NewLine (false);
				else
					pr_source_line++;
				if (!pr_file_p)
					return;
			}
			else
				pr_file_p++;
		}
		if (c == 0)
			return;		// end of file

	// skip // comments
		if (c=='/' && pr_file_p[1] == '/')
		{
			while (*pr_file_p && *pr_file_p != '\n')
				pr_file_p++;

			if (*pr_file_p == '\n')
				pr_file_p++;	//don't break on eof.
			if (!inhibitpreprocessor)
				QCC_PR_NewLine(false);
			else
				pr_source_line++;
			continue;
		}

	// skip /* */ comments
		if (c=='/' && pr_file_p[1] == '*')
		{
			pr_file_p+=1;
			do
			{
				pr_file_p++;
				if (pr_file_p[0]=='\n')
				{
					if (!inhibitpreprocessor)
						QCC_PR_NewLine(true);
					else
						pr_source_line++;
				}
				if (pr_file_p[1] == 0)
				{
					QCC_PR_ParseError(0, "EOF inside comment\n");
					pr_file_p++;
					return;
				}
			} while (pr_file_p[0] != '*' || pr_file_p[1] != '/');
			pr_file_p+=2;
			continue;
		}

		break;		// a real character has been found
	}
}

//============================================================================

#define	MAX_FRAMES	8192
char	pr_framemodelname[64];
char	pr_framemacros[MAX_FRAMES][64];
int		pr_framemacrovalue[MAX_FRAMES];
int		pr_nummacros, pr_oldmacros;
int		pr_macrovalue;
int		pr_savedmacro;

void QCC_PR_ClearGrabMacros (pbool newfile)
{
	if (!newfile)
		pr_nummacros = 0;
	pr_oldmacros = pr_nummacros;
	pr_macrovalue = 0;
	pr_savedmacro = -1;
}

int QCC_PR_FindMacro (char *name)
{
	int		i;

	for (i=pr_nummacros-1 ; i>=0 ; i--)
	{
		if (!STRCMP (name, pr_framemacros[i]))
		{
			return pr_framemacrovalue[i];
		}
	}
	for (i=pr_nummacros-1 ; i>=0 ; i--)
	{
		if (!stricmp (name, pr_framemacros[i]))
		{
			QCC_PR_ParseWarning(WARN_CASEINSENSITIVEFRAMEMACRO, "Case insensitive frame macro");
			return pr_framemacrovalue[i];
		}
	}
	return -1;
}

void QCC_PR_ExpandMacro(void)
{
	int		i = QCC_PR_FindMacro(pr_token);

	if (i < 0)
		QCC_PR_ParseError (ERR_BADFRAMEMACRO, "Unknown frame macro $%s", pr_token);

	QC_snprintfz(pr_token, sizeof(pr_token),"%d", i);
	pr_token_type = tt_immediate;
	pr_immediate_type = type_float;
	pr_immediate._float = (float)i;
}

// just parses text, returning false if an eol is reached
pbool QCC_PR_SimpleGetToken (void)
{
	int		c;
	int		i;

	pr_token[0] = 0;

// skip whitespace
	while ((c = *pr_file_p) && qcc_iswhite(c))
	{
		if (c=='\n')
			return false;
		pr_file_p++;
	}
	if (c == 0)	//eof
		return false;
	if (pr_file_p[0] == '/')
	{
		if (pr_file_p[1] == '/')
		{	//comment alert
			while(*pr_file_p && *pr_file_p != '\n')
				pr_file_p++;
			return false;
		}
		if (pr_file_p[1] == '*')
			return false;
	}

	i = 0;
	while ((c = *pr_file_p) && !qcc_iswhite(c) && c != ',' && c != ';' && c != ')' && c != '(' && c != ']' && !(c == '/' && pr_file_p[1] == '/'))
	{
		if (i == sizeof(qcc_token)-1)
			QCC_Error (ERR_INTERNAL, "token exceeds %i chars", i);
		pr_token[i] = c;
		i++;
		pr_file_p++;
	}
	pr_token[i] = 0;
	return i!=0;
}

pbool QCC_PR_LexMacroName(void)
{
	int		c;
	int		i;

	pr_token[0] = 0;

// skip whitespace
	while ((c = *pr_file_p) && qcc_iswhite(c))
	{
		if (c=='\n')
			return false;
		pr_file_p++;
	}
	if (!c)
		return false;
	if (pr_file_p[0] == '/')
	{
		if (pr_file_p[1] == '/')
		{	//comment alert
			while(*pr_file_p && *pr_file_p != '\n')
				pr_file_p++;
			return false;
		}
		if (pr_file_p[1] == '*')
			return false;
	}

	i = 0;
	while ( (c = *pr_file_p) > ' ' && c != '\n' && c != ',' && c != ';' && c != '&' && c != '|' && c != ')' && c != '(' && c != ']' && !(pr_file_p[0] == '.' && pr_file_p[1] == '.'))
	{
		if (i == sizeof(qcc_token)-1)
			QCC_Error (ERR_INTERNAL, "token exceeds %i chars", i);
		pr_token[i] = c;
		i++;
		pr_file_p++;
	}
	pr_token[i] = 0;
	return i!=0;
}

void QCC_PR_MacroFrame(char *name, int value)
{
	int i;
	for (i=pr_nummacros-1 ; i>=0 ; i--)
	{
		if (!STRCMP (name, pr_framemacros[i]))
		{
			pr_framemacrovalue[i] = value;
			if (i>=pr_oldmacros)
				QCC_PR_ParseWarning(WARN_DUPLICATEMACRO, "Duplicate macro defined (%s)", pr_token);
			//else it's from an old file, and shouldn't be mentioned.
			return;
		}
	}

	if (strlen(name)+1 > sizeof(pr_framemacros[0]))
		QCC_PR_ParseWarning(ERR_TOOMANYFRAMEMACROS, "Name for frame macro %s is too long", name);
	else
	{
		strcpy (pr_framemacros[pr_nummacros], name);
		pr_framemacrovalue[pr_nummacros] = value;
		pr_nummacros++;
		if (pr_nummacros >= MAX_FRAMES)
			QCC_PR_ParseError(ERR_TOOMANYFRAMEMACROS, "Too many frame macros defined");
	}
}

void QCC_PR_ParseFrame (void)
{
	while (QCC_PR_LexMacroName ())
	{
		QCC_PR_MacroFrame(pr_token, pr_macrovalue++);
	}
}

/*
==============
PR_LexGrab

Deals with counting sequence numbers and replacing frame macros
==============
*/
void QCC_PR_LexGrab (void)
{
	pr_file_p++;	// skip the $
//	if (!QCC_PR_SimpleGetToken ())
//		QCC_PR_ParseError ("hanging $");
	if (qcc_iswhite(*pr_file_p))
		QCC_PR_ParseError (ERR_BADFRAMEMACRO, "hanging $");
	QCC_PR_LexMacroName();
	if (!*pr_token)
		QCC_PR_ParseError (ERR_BADFRAMEMACRO, "hanging $");

// check for $frame
	if (!STRCMP (pr_token, "frame") || !STRCMP (pr_token, "framesave"))
	{
		QCC_PR_ParseFrame ();
		QCC_PR_Lex ();
	}
// ignore other known $commands - just for model/spritegen
	else if (!STRCMP (pr_token, "cd")
	|| !STRCMP (pr_token, "origin")
	|| !STRCMP (pr_token, "base")
	|| !STRCMP (pr_token, "flags")
	|| !STRCMP (pr_token, "scale")
	|| !STRCMP (pr_token, "skin") )
	{	// skip to end of line
		while (QCC_PR_LexMacroName ())
		;
		QCC_PR_Lex ();
	}
	else if (!STRCMP (pr_token, "flush"))
	{
		QCC_PR_ClearGrabMacros(true);
		while (QCC_PR_LexMacroName ())
		;
		QCC_PR_Lex ();
	}
	else if (!STRCMP (pr_token, "framevalue"))
	{
		QCC_PR_LexMacroName ();
		pr_macrovalue = atoi(pr_token);

		QCC_PR_Lex ();
	}
	else if (!STRCMP (pr_token, "framerestore"))
	{
		QCC_PR_LexMacroName ();
		QCC_PR_ExpandMacro();
		pr_macrovalue = (int)pr_immediate._float;

		QCC_PR_Lex ();
	}
	else if (!STRCMP (pr_token, "modelname"))
	{
		int i;
		QCC_PR_LexMacroName ();

		if (*pr_framemodelname)
			QCC_PR_MacroFrame(pr_framemodelname, pr_macrovalue);

		QC_strlcpy(pr_framemodelname, pr_token, sizeof(pr_framemodelname));

		i = QCC_PR_FindMacro(pr_framemodelname);
		if (i)
			pr_macrovalue = i;
		else
			i = 0;

		QCC_PR_Lex ();
	}
// look for a frame name macro
	else
		QCC_PR_ExpandMacro ();
}

//===========================
//compiler constants	- dmw

pbool QCC_PR_UndefineName(char *name)
{
//	int a;
	CompilerConstant_t *c;
	c = pHash_Get(&compconstantstable, name);
	if (!c)
	{
		QCC_PR_ParseWarning(WARN_UNDEFNOTDEFINED, "Precompiler constant %s was not defined", name);
		return false;
	}

	Hash_Remove(&compconstantstable, name);
	return true;
}

CompilerConstant_t *QCC_PR_DefineName(char *name)
{
	int i;
	CompilerConstant_t *cnst;

//	if (numCompilerConstants >= MAX_CONSTANTS)
//		QCC_PR_ParseError("Too many compiler constants - %i >= %i", numCompilerConstants, MAX_CONSTANTS);

	if (strlen(name) >= MAXCONSTANTNAMELENGTH || !*name)
		QCC_PR_ParseError(ERR_NAMETOOLONG, "Compiler constant name length is too long or short");

	cnst = pHash_Get(&compconstantstable, name);
	if (cnst)
	{
		QCC_PR_ParseWarning(WARN_DUPLICATEDEFINITION, "Duplicate definition for Precompiler constant %s", name);
		Hash_Remove(&compconstantstable, name);
	}

	cnst = qccHunkAlloc(sizeof(CompilerConstant_t));

	cnst->used = false;
	cnst->numparams = -1;
	cnst->evil = false;
	strcpy(cnst->name, name);
	cnst->namelen = strlen(name);
	cnst->value = cnst->name + strlen(cnst->name);
	for (i = 0; i < MAXCONSTANTPARAMS; i++)
		cnst->params[i][0] = '\0';

	pHash_Add(&compconstantstable, cnst->name, cnst, qccHunkAlloc(sizeof(bucket_t)));

	return cnst;
}

void QCC_PR_Undefine(void)
{
	QCC_PR_SimpleGetToken ();

	QCC_PR_UndefineName(pr_token);
//		QCC_PR_ParseError("%s was not defined.", pr_token);
}

void QCC_PR_PreProcessor_Define(pbool append)
{
	char *d;
	char *dbuf;
	int dbuflen;
	char *s;
	int quote=false;
	pbool preprocessorhack = false;
	CompilerConstant_t *cnst, *oldcnst;

	QCC_PR_SimpleGetToken ();

	if (!QCC_PR_SimpleGetToken ())
		QCC_PR_ParseError(ERR_NONAME, "No name defined for compiler constant");

	oldcnst = pHash_Get(&compconstantstable, pr_token);
	if (oldcnst)
		Hash_Remove(&compconstantstable, oldcnst->name);

	cnst = QCC_PR_DefineName(pr_token);

	if (*pr_file_p == '(')
	{
		cnst->numparams = 0;
		pr_file_p++;
		while(qcc_iswhitesameline(*pr_file_p))
			pr_file_p++;
		s = pr_file_p;
		for (;;)
		{
			if (*pr_file_p == ',' || *pr_file_p == ')')
			{
				int nl;
				nl = pr_file_p-s;
				while(qcc_iswhitesameline(s[nl]))
					nl--;
				if (cnst->numparams >= MAXCONSTANTPARAMS)
					QCC_PR_ParseError(ERR_MACROTOOMANYPARMS, "May not have more than %i parameters to a macro", MAXCONSTANTPARAMS);
				if (nl >= MAXCONSTANTPARAMLENGTH)
					QCC_PR_ParseError(ERR_MACROTOOMANYPARMS, "parameter name is too long (max %i)", MAXCONSTANTPARAMLENGTH);
				if (nl == 3 && s[0] == '.' && s[1] == '.' && s[2] == '.')
				{
					cnst->varg = true;
					if (*pr_file_p != ')')
						QCC_PR_ParseError(ERR_MACROTOOMANYPARMS, "varadic argument must be last");
				}
				else
				{
					memcpy(cnst->params[cnst->numparams], s, nl);
					cnst->params[cnst->numparams][nl] = '\0';
					cnst->numparams++;
				}
				if (*pr_file_p++ == ')')
					break;
				while(qcc_iswhitesameline(*pr_file_p))
					pr_file_p++;
				s = pr_file_p;
			}
			if(!*pr_file_p++)
			{
				QCC_PR_ParseError(ERR_EXPECTED, "missing ) in macro parameter list");
				break;
			}
		}
	}
	else cnst->numparams = -1;

	//disable append mode if they're trying to do something stupid
	if (append)
	{
		if (!oldcnst)
			append = false;	//append with no previous define is treated as just a regular define. huzzah.
		else if (cnst->numparams != oldcnst->numparams || cnst->varg != oldcnst->varg)
		{
			QCC_PR_ParseWarning(WARN_DUPLICATEPRECOMPILER, "different number of macro arguments in macro append");
			append = false;
		}
		else
		{
			int i;
			//arguments need to be specified, if only so that appends with arguments are still vaugely readable.
			//argument names need to match because the expansion is too lame to cope if they're different.
			for (i = 0; i < cnst->numparams; i++)
			{
				if (strcmp(cnst->params[i], oldcnst->params[i]))
					break;
			}
			if (i < cnst->numparams)
			{
				QCC_PR_ParseWarning(WARN_DUPLICATEPRECOMPILER, "arguments differ in macro append");
				append = false;
			}
			else
				append = true;
		}
	}

	s = pr_file_p;
	d = dbuf = NULL;
	dbuflen = 0;

	if (append)
	{
		//start with the old value
		int olen = strlen(oldcnst->value);
		dbuflen = olen + 128;
		dbuf = qccHunkAlloc(dbuflen);
		memcpy(dbuf, oldcnst->value, olen);
		d = dbuf + olen;
		*d++ = ' ';
	}

	while(*s == ' ' || *s == '\t')
		s++;
	while(1)
	{
		if ((d - dbuf) + 2 >= dbuflen)
		{
			int len = d - dbuf;
			dbuflen = (len+128) * 2;
			dbuf = qccHunkAlloc(dbuflen);
			memcpy(dbuf, d - len, len);
			d = dbuf + len;
		}

		if( *s == '\\' )
		{
			// read over a newline if necessary
			if( s[1] == '\n' || s[1] == '\r' )
			{
				char *exploitcheck;
				s++;
				QCC_PR_NewLine(false);
				s++;
				if( s[-1] == '\r' && s[0] == '\n' )
				{
					s++;
				}

/*
This began as a bug. It is still evil, but its oh so useful.

In C,
#define foobar \
foo\
bar\
moo

becomes foobarmoo, not foo\nbar\nmoo

#define hacks however, require that it becomes
foo\nbar\nmoo

# cannot be used on the first line of the macro, and then is only valid as the first non-white char of the following lines
so if present, the preceeding \\\n and following \\\n must become an actual \n instead of being stripped.
*/

				for (exploitcheck = s; *exploitcheck && qcc_iswhite(*exploitcheck); exploitcheck++)
					;
				if (*exploitcheck == '#')
				{
					*d++ = '\n';
					if (!cnst->evil)
						QCC_PR_ParseWarning(WARN_EVILPREPROCESSOR, "preprocessor directive within preprocessor macro %s", cnst->name);
					cnst->evil = true;
					preprocessorhack = true;
				}
				else if (preprocessorhack)
				{
					*d++ = '\n';
					preprocessorhack = false;
				}
			}
		}
		else if(*s == '\r' || *s == '\n' || *s == '\0')
		{
			break;
		}
		if (!quote && s[0]=='/'&&s[1]=='/')
			break;	//c++ style comments can just be ignored
		if (!quote && s[0]=='/'&&s[1]=='*')
		{	//multi-line c style comments become part of the define itself. this also negates the need for \ at the end of lines.
			//although we don't bother embedding.
			s+=2;
			for(;;)
			{
				if (!s[0])
				{
					QCC_PR_ParseWarning(WARN_DUPLICATEPRECOMPILER, "EOF inside quote in define %s", cnst->name);
					break;
				}
				if (s[0]=='*'&&s[1]=='/')
				{
					s+=2;
					break;
				}
				if (s[0] == '\n')
					pr_source_line++;
				s++;
			}
			continue;
		}
		if (*s == '\"')
			quote=!quote;

		*d = *s;
		d++;
		s++;
	}
	*d = '\0';

	cnst->value = dbuf;

	if (oldcnst && !append)
	{	//we always warn if it was already defined
		//we use different warning codes so that -Wno-mundane can be used to ignore identical redefinitions.
		if (strcmp(oldcnst->value, cnst->value))
			QCC_PR_ParseWarning(WARN_DUPLICATEPRECOMPILER, "Alternate precompiler definition of %s", pr_token);
		else
			QCC_PR_ParseWarning(WARN_IDENTICALPRECOMPILER, "Identical precompiler definition of %s", pr_token);
	}

	pr_file_p = s;
}

/* *buffer, *bufferlen and *buffermax should be NULL/0 at the start */
static void QCC_PR_ExpandStrCat(char **buffer, size_t *bufferlen, size_t *buffermax,   char *newdata, size_t newlen)
{
	size_t newmax = *bufferlen + newlen;
	if (newmax < *bufferlen)//check for overflow
	{
		QCC_PR_ParseWarning(ERR_INTERNAL, "exceeds 4gb");
		return;
	}
	if (newmax > *buffermax)
	{
		char *newbuf;
		if (newmax < 64)
			newmax = 64;
		if (newmax < *bufferlen * 2)
		{
			newmax = *bufferlen * 2;
			if (newmax < *bufferlen) /*overflowed?*/
			{
				QCC_PR_ParseWarning(ERR_INTERNAL, "exceeds 4gb");
				return;
			}
		}
		newbuf = realloc(*buffer, newmax);
		if (!newbuf)
		{
			QCC_PR_ParseWarning(ERR_INTERNAL, "out of memory");
			return; /*OOM*/
		}
		*buffer = newbuf;
		*buffermax = newmax;
	}
	memcpy(*buffer + *bufferlen, newdata, newlen);
	*bufferlen += newlen;
	/*no null terminator, remember to cat one if required*/
}

static char *QCC_PR_CheckBuiltinCompConst(char *constname, char *retbuf, size_t retbufsize)
{
	if (!strcmp(constname, "__TIME__"))
	{
		time_t long_time;
		time( &long_time );
		strftime( retbuf, retbufsize,	"\"%H:%M\"", localtime( &long_time ));
		return retbuf;
	}
	if (!strcmp(constname, "__DATE__"))
	{
		time_t long_time;
		time( &long_time );
		strftime( retbuf, retbufsize,	"\"%a %d %b %Y\"", localtime( &long_time ));
		return retbuf;
	}
	if (!strcmp(constname, "__RAND__"))
	{
		QC_snprintfz(retbuf, retbufsize, "%i", rand());
		return retbuf;
	}
	if (!strcmp(constname, "__QCCVER__"))
	{
		return "FTEQCC "__DATE__","__TIME__"";
	}
	if (!strcmp(constname, "__FILE__"))
	{
		QC_snprintfz(retbuf, retbufsize, "\"%s\"", strings + s_file);
		return retbuf;
	}
	if (!strcmp(constname, "__LINE__"))
	{
		QC_snprintfz(retbuf, retbufsize, "%i", pr_source_line);
		return retbuf;
	}
	if (!strcmp(constname, "__LINESTR__"))
	{
		QC_snprintfz(retbuf, retbufsize, "\"%i\"", pr_source_line);
		return retbuf;
	}
	if (!strcmp(constname, "__FUNC__"))
	{
		QC_snprintfz(retbuf, retbufsize, "\"%s\"",pr_scope?pr_scope->name:"<NO FUNCTION>");
		return retbuf;
	}
	if (!strcmp(constname, "__NULL__"))
	{
		return "0i";
	}
	return NULL;	//didn't match
}

#define PASTE2(a,b) a##b
#define PASTE(a,b) PASTE2(a,b)
#define STRINGIFY2(a) #a
#define STRINGIFY(a) STRINGIFY2(a)
#define spam(x) /*static float PASTE(spam,__LINE__);*/ if (PASTE2(spam,__LINE__) != x) {dprint(#x " chaned in " __FILE__ " on line " STRINGIFY2(__LINE__) "\n");  PASTE2(spam,__LINE__) = x;}
#define dprint printf

int QCC_PR_CheckCompConst(void)
{
	char		*oldpr_file_p = pr_file_p;
	int whitestart = 5;

	CompilerConstant_t *c;

	char *end, *tok;
	char retbuf[256];

//	spam(whitestart);

	for (end = pr_file_p; ; end++)
	{
		if (!*end || qcc_iswhite(*end))
			break;

		if (*end == ')'
			||	*end == '('
			||	*end == '+'
			||	*end == '-'
			||	*end == '*'
			||	*end == '/'
			||	*end == '|'
			||	*end == '&'
			||	*end == '='
			||	*end == '^'
			||	*end == '~'
			||	*end == '['
			||	*end == ']'
			||	*end == '\"'
			||	*end == '{'
			||	*end == '}'
			||	*end == ';'
			||	*end == ':'
			||	*end == ','
			||	*end == '.'
			||	*end == '#')
				break;
	}
	QC_strnlcpy(pr_token, pr_file_p, end-pr_file_p, sizeof(pr_token));

//	printf("%s\n", pr_token);
	c = pHash_Get(&compconstantstable, pr_token);

	if (c && !c->inside)
	{
		pr_file_p = oldpr_file_p+strlen(c->name);
		while(*pr_file_p == ' ' || *pr_file_p == '\t')
			pr_file_p++;
		if (c->numparams>=0)
		{
			if (*pr_file_p == '(')
			{
				int p;
				char *start;
				char *starttok;
				char *buffer;
				size_t buffermax;
				size_t bufferlen;
				char *paramoffset[MAXCONSTANTPARAMS+1];
				int param=0;
				int plevel=0;
				pbool noargexpand;

				pr_file_p++;
				QCC_PR_LexWhitespace(false);
				start = pr_file_p;
				while(1)
				{
					// handle strings correctly by ignoring them
					if (*pr_file_p == '\"')
					{
						do {
							pr_file_p++;
						} while( (pr_file_p[-1] == '\\' || pr_file_p[0] != '\"') && *pr_file_p && *pr_file_p != '\n' );
					}
					if (*pr_file_p == '(')
						plevel++;
					else if (!plevel && (*pr_file_p == ',' || *pr_file_p == ')'))
					{
						if (*pr_file_p == ',' && c->varg && param >= c->numparams)
							;	//skip extra trailing , arguments if we're varging.
						else
						{
							paramoffset[param++] = start;
							start = pr_file_p+1;
							if (*pr_file_p == ')')
							{
								*pr_file_p = '\0';
								pr_file_p++;
								break;
							}
							*pr_file_p = '\0';
							pr_file_p++;
							QCC_PR_LexWhitespace(false);
							start = pr_file_p;
							// move back by one char because we move forward by one at the end of the loop
							pr_file_p--;
							if (param == MAXCONSTANTPARAMS || param > c->numparams)
								QCC_PR_ParseError(ERR_TOOMANYPARAMS, "Too many parameters in macro call");
						}
					} else if (*pr_file_p == ')' )
						plevel--;
					else if(*pr_file_p == '\n')
						QCC_PR_NewLine(false);

					// see that *pr_file_p = '\0' up there? Must ++ BEFORE checking for !*pr_file_p
					pr_file_p++;
					if (!*pr_file_p)
						QCC_PR_ParseError(ERR_EOF, "EOF on macro call");
				}
				if (param < c->numparams)
					QCC_PR_ParseError(ERR_TOOFEWPARAMS, "Not enough macro parameters");
				paramoffset[param] = start;

				buffer = NULL;
				bufferlen = 0;
				buffermax = 0;

				oldpr_file_p = pr_file_p;
				pr_file_p = c->value;
				for(;;)
				{
					noargexpand = false;
					whitestart = bufferlen;
					starttok = pr_file_p;
					/*while(qcc_iswhite(*pr_file_p))	//copy across whitespace
					{
						if (!*pr_file_p)
							break;
						pr_file_p++;
					}*/
					QCC_PR_LexWhitespace(true);
					if (starttok != pr_file_p)
					{
						QCC_PR_ExpandStrCat(&buffer, &bufferlen, &buffermax,   starttok, pr_file_p - starttok);
					}

					if(*pr_file_p == '\"')
					{
						starttok = pr_file_p;
						do
						{
							pr_file_p++;
						} while( (pr_file_p[-1] == '\\' || pr_file_p[0] != '\"') && *pr_file_p && *pr_file_p != '\n' );
						if(*pr_file_p == '\"')
							pr_file_p++;

						QCC_PR_ExpandStrCat(&buffer, &bufferlen, &buffermax,   starttok, pr_file_p - starttok);
						continue;
					}
					else if (*pr_file_p == '#')	//if you ask for #a##b you will be shot. use #a #b instead, or chain macros.
					{
						if (pr_file_p[1] == '#')
						{	//concatinate (strip out whitespace before the token)
							bufferlen = whitestart;
							pr_file_p+=2;
							noargexpand = true;
						}
						else
						{	//stringify
							pr_file_p++;
							pr_file_p = QCC_COM_Parse2(pr_file_p);
							if (!pr_file_p)
								break;

							for (p = 0; p < param; p++)
							{
								if (!STRCMP(qcc_token, c->params[p]))
								{
									QCC_PR_ExpandStrCat(&buffer, &bufferlen, &buffermax,   "\"", 1);
									QCC_PR_ExpandStrCat(&buffer, &bufferlen, &buffermax,   paramoffset[p], strlen(paramoffset[p]));
									QCC_PR_ExpandStrCat(&buffer, &bufferlen, &buffermax,   "\"", 1);
									break;
								}
							}
							if (p == param)
							{
								QCC_PR_ExpandStrCat(&buffer, &bufferlen, &buffermax,   "#", 1);
								QCC_PR_ExpandStrCat(&buffer, &bufferlen, &buffermax,   qcc_token, strlen(qcc_token));
								if (!c->evil)
									QCC_PR_ParseWarning(0, "'#' is not followed by a macro parameter in %s", c->name);
							}
							continue;	//already did this one
						}
					}

					end = qcc_token;
					pr_file_p = QCC_COM_Parse2(pr_file_p);
					if (!pr_file_p)
						break;

					for (p = 0; p < c->numparams; p++)
					{
						if (!STRCMP(qcc_token, c->params[p]))
						{
							char *argstart, *argend;

							for (start = pr_file_p; qcc_iswhite(*start); start++)
								;
							if (noargexpand || (start[0] == '#' && start[1] == '#'))
								QCC_PR_ExpandStrCat(&buffer, &bufferlen, &buffermax,   paramoffset[p], strlen(paramoffset[p]));
							else
							{
								for (argstart = paramoffset[p]; *argstart; argstart = argend)
								{
									argend = argstart;
									while (qcc_iswhite(*argend))
										argend++;
									if (*argend == '\"')
									{
										do
										{
											argend++;
										} while( (argend[-1] == '\\' || argend[0] != '\"') && *argend && *argend != '\n' );
										if(*argend == '\"')
											argend++;
										end = NULL;
									}
									else
									{
										argend = QCC_COM_Parse2(argend);
										if (!argend)
											break;
										end = QCC_PR_CheckBuiltinCompConst(qcc_token, retbuf, sizeof(retbuf));
									}
									//FIXME: we should be testing all defines instead of just built-in ones.
									if (end)
										QCC_PR_ExpandStrCat(&buffer, &bufferlen, &buffermax,   end, strlen(end));
									else
										QCC_PR_ExpandStrCat(&buffer, &bufferlen, &buffermax,   argstart, argend-argstart);
								}
							}
							break;
						}
					}
					if (c->varg && !STRCMP(qcc_token, "__VA_ARGS__"))
						QCC_PR_ExpandStrCat(&buffer, &bufferlen, &buffermax,   paramoffset[c->numparams], strlen(paramoffset[c->numparams]));
					else if (p == c->numparams)
						QCC_PR_ExpandStrCat(&buffer, &bufferlen, &buffermax,   qcc_token, strlen(qcc_token));
				}

				for (p = 0; p < param-1; p++)
					paramoffset[p][strlen(paramoffset[p])] = ',';
				paramoffset[p][strlen(paramoffset[p])] = ')';

				pr_file_p = oldpr_file_p;
				if (!bufferlen)
					expandedemptymacro = true;
				else
				{
					QCC_PR_ExpandStrCat(&buffer, &bufferlen, &buffermax,   "\0", 1);
					QCC_PR_IncludeChunkEx(buffer, true, NULL, c);
				}
				expandedemptymacro = true;
				free(buffer);

				if (flag_debugmacros)
				{
					if (flag_msvcstyle)
						printf ("%s(%i) : macro %s: %s\n", strings+s_file, pr_source_line, c->name, pr_file_p);
					else
						printf ("%s:%i: macro %s: %s\n", strings+s_file, pr_source_line, c->name, pr_file_p);
				}
			}
			else
				QCC_PR_ParseError(ERR_TOOFEWPARAMS, "Macro without argument list");
		}
		else
		{
			if (*c->value)
				QCC_PR_IncludeChunkEx(c->value, false, NULL, c);
			expandedemptymacro = true;
		}
		return true;
	}

	tok = QCC_PR_CheckBuiltinCompConst(pr_token, retbuf, sizeof(retbuf));
	if (tok)
	{
		pr_file_p = end;
		QCC_PR_IncludeChunkEx(tok, true, NULL, NULL);
		return true;
	}
	return false;
}

char *QCC_PR_CheckCompConstString(char *def)
{
	char *s;

	CompilerConstant_t *c;

	c = pHash_Get(&compconstantstable, def);

	if (c)
	{
		s = QCC_PR_CheckCompConstString(c->value);
		return s;
	}
	return def;
}

CompilerConstant_t *QCC_PR_CheckCompConstDefined(char *def)
{
	CompilerConstant_t *c = pHash_Get(&compconstantstable, def);
	return c;
}

char *QCC_PR_CheckCompConstTooltip(char *word, char *outstart, char *outend)
{
	int i;
	CompilerConstant_t *c = QCC_PR_CheckCompConstDefined(word);
	if (c)
	{
		char *out = outstart;
		if (c->numparams >= 0)
		{
			QC_snprintfz(out, outend-out, "#define %s(", c->name);
			out += strlen(out);
			for (i = 0; i < c->numparams-1; i++)
			{
				QC_snprintfz(out, outend-out, "%s,", c->params[i]);
				out += strlen(out);
			}
			if (i < c->numparams)
			{
				QC_snprintfz(out, outend-out, "%s", c->params[i]);
				out += strlen(out);
			}
			QC_snprintfz(out, outend-out, ")");
		}
		else
			QC_snprintfz(out, outend-out, "#define %s", c->name);
		out += strlen(out);
		if (c->value && *c->value)
			QC_snprintfz(out, outend-out, "\n%s", c->value);

		return outstart;
	}
	return NULL;
}

//============================================================================

/*
==============
PR_Lex

Sets pr_token, pr_token_type, and possibly pr_immediate and pr_immediate_type
==============
*/
void QCC_PR_Lex (void)
{
	int		c;

	pr_token[0] = 0;

	if (!pr_file_p)
	{
		if (QCC_PR_UnInclude())
		{
			QCC_PR_Lex();
			return;
		}
		pr_token_type = tt_eof;
		return;
	}

	QCC_PR_LexWhitespace (false);

	pr_token_line_last = pr_token_line;
	pr_token_line = pr_source_line;

	if (!pr_file_p)
	{
		if (QCC_PR_UnInclude())
		{
			QCC_PR_Lex();
			return;
		}
		pr_token_type = tt_eof;
		return;
	}

	c = *pr_file_p;

	if (!c)
	{
		if (QCC_PR_UnInclude())
		{
			QCC_PR_Lex();
			return;
		}
		pr_token_type = tt_eof;
		return;
	}

// handle quoted strings as a unit
	if (c == '\"' || (c == 'R' && pr_file_p[1] == '\"'))
	{
		QCC_PR_LexString ();
		return;
	}

// handle quoted vectors as a unit
	if (c == '\'')
	{
		QCC_PR_LexVector ();
		return;
	}

// if the first character is a valid identifier, parse until a non-id
// character is reached
	if ((c == '%') && pr_file_p[1] >= '0' && pr_file_p[1] <= '9')	//let's see which one we make into an operator first... possibly both...
	{
		QCC_PR_ParseWarning(0, "%% prefixes to denote integers are deprecated. Please use a postfix of 'i'");
		pr_file_p++;
		pr_token_type = tt_immediate;
		pr_immediate_type = type_integer;
		pr_immediate._int = QCC_PR_LexInteger ();
		return;
	}
	if ( c == '0' && pr_file_p[1] == 'x')
	{
		pr_token_type = tt_immediate;
		QCC_PR_LexNumber();
		return;
	}
	if ( (c == '.'&&pr_file_p[1]!='.'&&pr_file_p[1] >='0' && pr_file_p[1] <= '9') || (c >= '0' && c <= '9') || ( c=='-' && pr_file_p[1]>='0' && pr_file_p[1] <='9') )
	{
		pr_token_type = tt_immediate;
		QCC_PR_LexNumber ();
		return;
	}

	if (c == '#' && !(pr_file_p[1]=='\"' || pr_file_p[1]=='-' || (pr_file_p[1]>='0' && pr_file_p[1] <='9')))	//hash and not number
	{
		pr_file_p++;
		if (!QCC_PR_CheckCompConst())
		{
			if (!QCC_PR_SimpleGetToken())
				strcpy(pr_token, "unknown");
			QCC_PR_ParseError(ERR_CONSTANTNOTDEFINED, "Explicit precompiler usage when not defined %s", pr_token);
		}
		else
		{
			QCC_PR_Lex();
			if (pr_token_type == tt_eof)
				QCC_PR_Lex();
		}

		return;
	}

	if ( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || (c & 0x80))
	{
		if (flag_hashonly || !QCC_PR_CheckCompConst())	//look for a macro.
			QCC_PR_LexName ();
		else
		{
			//we expanded a macro. we need to read the tokens out of it now though
			QCC_PR_Lex();
			if (pr_token_type == tt_eof)
			{
				if (QCC_PR_UnInclude())
				{
					QCC_PR_Lex();
					return;
				}
				pr_token_type = tt_eof;
			}
		}
		return;
	}

	if (c == '$')
	{
		QCC_PR_LexGrab ();
		return;
	}

// parse symbol strings until a non-symbol is found
	QCC_PR_LexPunctuation ();
}

//=============================================================================

pbool QCC_Temp_Describe(QCC_def_t *def, char *buffer, int buffersize);
void QCC_PR_ParsePrintDef (int type, QCC_def_t *def)
{
	if (!qccwarningaction[type])
		return;
	if (def->s_file)
	{
		char tybuffer[512];
		char tmbuffer[512];
		if (QCC_Temp_Describe(def, tmbuffer, sizeof(tmbuffer)))
		{
			printf ("%s:%i:    (%s)(%s)\n", strings + def->s_file, def->s_line, TypeName(def->type, tybuffer, sizeof(tybuffer)), tmbuffer);
		}
		else
		{
			if (flag_msvcstyle)
				printf ("%s(%i) :    %s %s  is defined here\n", strings + def->s_file, def->s_line, TypeName(def->type, tybuffer, sizeof(tybuffer)), def->name);
			else
				printf ("%s:%i:    %s %s  is defined here\n", strings + def->s_file, def->s_line, TypeName(def->type, tybuffer, sizeof(tybuffer)), def->name);
		}
	}
}
void QCC_PR_ParsePrintSRef (int type, QCC_sref_t def)
{
	QCC_PR_ParsePrintDef(type, def.sym);
}

void *errorscope;
void QCC_PR_PrintScope (void)
{
	if (pr_scope)
	{
		if (errorscope != pr_scope)
			printf ("in function %s (line %i),\n", pr_scope->name, pr_scope->line);
		errorscope = pr_scope;
	}
	else
	{
		if (errorscope)
			printf ("at global scope,\n");
		errorscope = NULL;
	}
}
void QCC_PR_ResetErrorScope(void)
{
	errorscope = NULL;
}
/*
============
PR_ParseError

Aborts the current file load
============
*/
#ifndef QCC
void editbadfile(char *file, int line);
#endif
//will abort.
void VARGS QCC_PR_ParseError (int errortype, const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,error);
	QC_vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

#ifndef QCC
	editbadfile(strings+s_file, pr_source_line);
#endif

	QCC_PR_PrintScope();
	if (flag_msvcstyle)
		printf ("%s(%i) : error: %s\n", strings + s_file, pr_source_line, string);
	else
		printf ("%s:%i: error: %s\n", strings + s_file, pr_source_line, string);

	longjmp (pr_parse_abort, 1);
}
//will abort.
void VARGS QCC_PR_ParseErrorPrintDef (int errortype, QCC_def_t *def, const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,error);
	QC_vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

#ifndef QCC
	editbadfile(strings+s_file, pr_source_line);
#endif
	QCC_PR_PrintScope();
	if (flag_msvcstyle)
		printf ("%s(%i) : error: %s\n", strings + s_file, pr_source_line, string);
	else
		printf ("%s:%i: error: %s\n", strings + s_file, pr_source_line, string);

	QCC_PR_ParsePrintDef(WARN_ERROR, def);

	longjmp (pr_parse_abort, 1);
}

void VARGS QCC_PR_ParseErrorPrintSRef (int errortype, QCC_sref_t def, const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,error);
	QC_vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

#ifndef QCC
	editbadfile(strings+s_file, pr_source_line);
#endif
	QCC_PR_PrintScope();
	if (flag_msvcstyle)
		printf ("%s(%i) : error: %s\n", strings + s_file, pr_source_line, string);
	else
		printf ("%s:%i: error: %s\n", strings + s_file, pr_source_line, string);

	QCC_PR_ParsePrintSRef(WARN_ERROR, def);

	longjmp (pr_parse_abort, 1);
}

pbool VARGS QCC_PR_PrintWarning (int type, const char *file, int line, const char *string)
{
	char *wnam = QCC_NameForWarning(type);
	if (!wnam)
		wnam = "";

	QCC_PR_PrintScope();
	if (type >= ERR_PARSEERRORS)
	{
		if (!file || !*file)
			printf (":: error%s: %s\n", wnam, string);
		else if (flag_msvcstyle)
			printf ("%s(%i) : error%s: %s\n", file, line, wnam, string);
		else
			printf ("%s:%i: error%s: %s\n", file, line, wnam, string);
		pr_error_count++;
	}
	else if (qccwarningaction[type] == 2)
	{	//-werror
		if (!file || !*file)
			printf (": werror%s: %s\n", wnam, string);
		else if (flag_msvcstyle)
			printf ("%s(%i) : werror%s: %s\n", file, line, wnam, string);
		else
			printf ("%s:%i: werror%s: %s\n", file, line, wnam, string);
		pr_error_count++;
	}
	else
	{
		if (!file || !*file)
			printf (": warning%s: %s\n", wnam, string);
		else if (flag_msvcstyle)
			printf ("%s(%i) : warning%s: %s\n", file, line, wnam, string);
		else
			printf ("%s:%i: warning%s: %s\n", file, line, wnam, string);
		pr_warning_count++;
	}
	return true;
}
pbool VARGS QCC_PR_Warning (int type, const char *file, int line, const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	if (!qccwarningaction[type])
		return false;

	va_start (argptr,error);
	QC_vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

	return QCC_PR_PrintWarning(type, file, line, string);
}

//can be used for errors, qcc execution will continue.
pbool VARGS QCC_PR_ParseWarning (int type, const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	if (!qccwarningaction[type])
		return false;

	va_start (argptr,error);
	QC_vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

	return QCC_PR_PrintWarning(type, strings + s_file, pr_source_line, string);
}

void VARGS QCC_PR_Note (int type, const char *file, int line, const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	if (!qccwarningaction[type])
		return;

	va_start (argptr,error);
	QC_vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

	QCC_PR_PrintScope();
	if (!file)
		printf ("note: %s\n", string);
	else if (flag_msvcstyle)
		printf ("%s(%i) : note: %s\n", file, line, string);
	else
		printf ("%s:%i: note: %s\n", file, line, string);
}


/*
=============
PR_Expect

Issues an error if the current token isn't equal to string
Gets the next token
=============
*/
#ifndef COMMONINLINES
void QCC_PR_Expect (const char *string)
{
	if (STRCMP (string, pr_token))
		QCC_PR_ParseError (ERR_EXPECTED, "expected %s, found %s",string, pr_token);
	QCC_PR_Lex ();
}
#endif

pbool QCC_PR_CheckTokenComment(const char *string, char **comment)
{
	char c;
	char *start;
	int nl;
	char *old;
	int oldlen;
	pbool replace = true;
	pbool nextcomment = true;

	if (pr_token_type != tt_punct)
		return false;

	if (STRCMP (string, pr_token))
		return false;

	if (comment)
	{
		// skip whitespace
		nl = false;
		while(nextcomment)
		{
			nextcomment = false;

			while ((c = *pr_file_p) && qcc_iswhite(c))
			{
				if (c=='\n')	//allow new lines, but only if there's whitespace before any tokens, and no double newlines.
				{
					if (nl)
					{
						pr_file_p++;
						QCC_PR_NewLine(false);
						break;
					}
					nl = true;
				}
				else
				{
					pr_file_p++;
					nl = false;
				}
			}
			if (nl)
				break;

			// parse // comments
			if (c=='/' && pr_file_p[1] == '/')
			{
				pr_file_p += 2;
				while (*pr_file_p == ' ' || *pr_file_p == '\t')
					pr_file_p++;
				start = pr_file_p;
				while (*pr_file_p && *pr_file_p != '\n')
					pr_file_p++;

				if (*pr_file_p == '\n')
				{
					pr_file_p++;
					QCC_PR_NewLine(false);
				}

				old = replace?NULL:*comment;
				replace = false;
				oldlen = old?strlen(old)+1:0;
				*comment = qccHunkAlloc(oldlen + (pr_file_p-start)+1);
				if (oldlen)
				{
					memcpy(*comment, old, oldlen-1);
					memcpy(*comment+oldlen-1, "\n", 1);
				}
				memcpy(*comment + oldlen, start, pr_file_p - start);
				oldlen = oldlen+pr_file_p - start;
				while(oldlen > 0 && ((*comment)[oldlen-1] == '\r' || (*comment)[oldlen-1] == '\n' || (*comment)[oldlen-1] == '\t' || (*comment)[oldlen-1] == ' '))
					oldlen--;
				(*comment)[oldlen] = 0;
				nextcomment = true;	//include the next // too
				nl = true;
			}
			// parse /* comments
			else if (c=='/' && pr_file_p[1] == '*' && replace)
			{
				pr_file_p+=1;
				start = pr_file_p+1;

				do
				{
					pr_file_p++;
					if (pr_file_p[0]=='\n')
					{
						QCC_PR_NewLine(true);
					}
					else if (pr_file_p[1] == 0)
					{
						QCC_PR_ParseError(0, "EOF inside comment\n");
						break;
					}
				} while (pr_file_p[0] != '*' || pr_file_p[1] != '/');

				if (pr_file_p[1] == 0)
					break;

				old = replace?NULL:*comment;
				replace = false;
				oldlen = old?strlen(old):0;
				*comment = qccHunkAlloc(oldlen + (pr_file_p-start)+1);
				memcpy(*comment, old, oldlen);
				memcpy(*comment + oldlen, start, pr_file_p - start);
				(*comment)[oldlen+pr_file_p - start] = 0;

				pr_file_p+=2;
			}
		}
	}

	//and then do the rest properly.
	QCC_PR_Lex ();
	return true;
}
/*
=============
PR_Check

Returns true and gets the next token if the current token equals string
Returns false and does nothing otherwise
=============
*/
#ifndef COMMONINLINES
pbool QCC_PR_CheckToken (const char *string)
{
	if (pr_token_type != tt_punct)
		return false;

	if (STRCMP (string, pr_token))
		return false;

	QCC_PR_Lex ();
	return true;
}

pbool QCC_PR_CheckImmediate (const char *string)
{
	if (pr_token_type != tt_immediate)
		return false;

	if (STRCMP (string, pr_token))
		return false;

	QCC_PR_Lex ();
	return true;
}

pbool QCC_PR_CheckName(const char *string)
{
	if (pr_token_type != tt_name)
		return false;
	if (flag_caseinsensitive)
	{
		if (stricmp (string, pr_token))
			return false;
	}
	else
	{
		if (STRCMP(string, pr_token))
			return false;
	}
	QCC_PR_Lex ();
	return true;
}

pbool QCC_PR_CheckKeyword(int keywordenabled, const char *string)
{
	if (!keywordenabled)
		return false;
	if (flag_caseinsensitive)
	{
		if (stricmp (string, pr_token))
			return false;
	}
	else
	{
		if (STRCMP(string, pr_token))
			return false;
	}
	QCC_PR_Lex ();
	return true;
}
#endif


/*
============
PR_ParseName

Checks to see if the current token is a valid name
============
*/
char *QCC_PR_ParseName (void)
{
	char	ident[MAX_NAME];
	char *ret;

	if (pr_token_type != tt_name)
	{
		if (pr_token_type == tt_eof)
			QCC_PR_ParseError (ERR_EOF, "unexpected EOF", pr_token);
		else if (strcmp(pr_token, "..."))	//seriously? someone used '...' as an intrinsic NAME?
			QCC_PR_ParseError (ERR_NOTANAME, "\"%s\" - not a name", pr_token);
	}
	if (strlen(pr_token) >= MAX_NAME-1)
		QCC_PR_ParseError (ERR_NAMETOOLONG, "name too long");
	strcpy (ident, pr_token);
	QCC_PR_Lex ();

	ret = qccHunkAlloc(strlen(ident)+1);
	strcpy(ret, ident);
	return ret;
//	return ident;
}

/*
============
PR_FindType

Returns a preexisting complex type that matches the parm, or allocates
a new one and copies it out.
============
*/

//requires EVERYTHING to be the same
static int typecmp_strict(QCC_type_t *a, QCC_type_t *b)
{
	int i;
	if (a == b)
		return 0;
	if (!a || !b)
		return 1;	//different (^ and not both null)

	if (a->type != b->type)
		return 1;
	if (a->num_parms != b->num_parms)
		return 1;
	if (a->vargs != b->vargs)
		return 1;
	if (a->vargcount != b->vargcount)
		return 1;

	if (a->size != b->size)
		return 1;

	if (a->accessors != b->accessors)
		return 1;

	if (STRCMP(a->name, b->name))
		return 1;

	if (typecmp_strict(a->aux_type, b->aux_type))
		return 1;

	i = a->num_parms;
	while(i-- > 0)
	{
		if (STRCMP(a->params[i].paramname, b->params[i].paramname))
			return 1;
		if (typecmp_strict(a->params[i].type, b->params[i].type))
			return 1;
	}

	return 0;
}

//reports if they're functionally equivelent (allows assignments) 
int typecmp(QCC_type_t *a, QCC_type_t *b)
{
	int i;
	if (a == b)
		return 0;
	if (!a || !b)
		return 1;	//different (^ and not both null)

	if (a->type != b->type)
		return 1;
	if (a->num_parms != b->num_parms)
		return 1;
	if (a->vargs != b->vargs)
		return 1;
	if (a->vargcount != b->vargcount)
		return 1;

	if (a->size != b->size)
		return 1;

	if ((a->type == ev_entity && a->parentclass) || a->type == ev_struct || a->type == ev_union)
	{
		if (STRCMP(a->name, b->name))
			return 1;
	}

	if (typecmp(a->aux_type, b->aux_type))
		return 1;

	i = a->num_parms;
	while(i-- > 0)
	{
		if (a->type != ev_function && STRCMP(a->params[i].paramname, b->params[i].paramname))
			return 1;
		if (typecmp(a->params[i].type, b->params[i].type))
			return 1;
	}

	return 0;
}

//compares the types, but doesn't complain if there are optional arguments which differ
int typecmp_lax(QCC_type_t *a, QCC_type_t *b)
{
	unsigned int minargs = 0;
	unsigned int t;

	if (a == b)
		return 0;
	if (!a || !b)
		return 1;	//different (^ and not both null)

	if (a->type != b->type)
	{
		if (a->type == ev_accessor && a->parentclass)
			if (!typecmp_lax(a->parentclass, b))
				return 0;
		if (b->type == ev_accessor && b->parentclass)
			if (!typecmp_lax(a, b->parentclass))
				return 0;
		if (a->type != ev_variant && b->type != ev_variant)
			return 1;
	}
	else if (a->size != b->size)
		return 1;

	if (a->vargcount != b->vargcount)
		return 1;

	t = a->num_parms;
	minargs = t;
	t = b->num_parms;
	if (minargs > t)
		minargs = t;

//	if (STRCMP(a->name, b->name))	//This isn't 100% clean.
//		return 1;

	if (typecmp_lax(a->aux_type, b->aux_type))
		return 1;

	//variants and args don't make sense, and are considered equivelent(ish).
	if (a->type == ev_variant || b->type == ev_variant)
		return 0;

	//optional arg types must match, even if they're not specified in one.
	for (t = 0; t < minargs; t++)
	{
		if (a->params[t].type->type != b->params[t].type->type)
			return 1;
		//classes/structs/unions are matched on class names rather than the contents of the class
		//it gets too painful otherwise, with recursive definitions.
		if (a->params[t].type->type == ev_entity || a->params[t].type->type == ev_struct || a->params[t].type->type == ev_union)
		{
			if (STRCMP(a->params[t].type->name, b->params[t].type->name))
				return 1;
		}
		else
		{
			if (typecmp_lax(a->params[t].type, b->params[t].type))
				return 1;
		}
	}
	if (a->num_parms > minargs)
	{
		for (t = minargs; t < a->num_parms; t++)
		{
			if (!a->params[t].optional)
				return 1;
		}
	}
	if (b->num_parms > minargs)
	{
		for (t = minargs; t < b->num_parms; t++)
		{
			if (!b->params[t].optional)
				return 1;
		}
	}

	return 0;
}


QCC_type_t *QCC_PR_DuplicateType(QCC_type_t *in, pbool recurse)
{
	QCC_type_t *out;
	if (!in)
		return NULL;

	out = QCC_PR_NewType(in->name, in->type, false);
	out->aux_type = recurse?QCC_PR_DuplicateType(in->aux_type, recurse):in->aux_type;
	out->num_parms = in->num_parms;
	out->params = qccHunkAlloc(sizeof(*out->params) * out->num_parms);
	memcpy(out->params, in->params, sizeof(*out->params) * out->num_parms);
	out->size = in->size;
	out->num_parms = in->num_parms;
	out->name = in->name;
	out->parentclass = in->parentclass;

	return out;
}

static void Q_strlcat(char *dest, const char *src, int sizeofdest)
{
	if (sizeofdest)
	{
		int dlen = strlen(dest);
		int slen = strlen(src)+1;
		memcpy(dest+dlen, src, min((sizeofdest-1)-dlen, slen));
		dest[sizeofdest - 1] = 0;
	}
}

char *TypeName(QCC_type_t *type, char *buffer, int buffersize)
{
	char *ret;

	if (type->type == ev_void)
	{
		if (buffersize < 0)
			return buffer;
		*buffer = 0;
		Q_strlcat(buffer, "void", buffersize);
		return buffer;
	}

	if (type->type == ev_pointer)
	{
		if (buffersize < 0)
			return buffer;
		TypeName(type->aux_type, buffer, buffersize-2);
		Q_strlcat(buffer, " *", buffersize);
		return buffer;
	}

	ret = buffer;
	if (type->type == ev_field)
	{
		type = type->aux_type;
		*ret++ = '.';
	}
	*ret = 0;

	if (type->type == ev_function)
	{
		int args = type->num_parms;
		pbool vargs = type->vargs;
		unsigned int i;
		Q_strlcat(buffer, type->aux_type->name, buffersize);
		Q_strlcat(buffer, "(", buffersize);
		for (i = 0; i < type->num_parms; )
		{
			if (type->params[i].optional)
				Q_strlcat(buffer, "optional ", buffersize);
			args--;

			Q_strlcat(buffer, type->params[i].type->name, buffersize);
			if (type->params[i].paramname && *type->params[i].paramname)
			{
				Q_strlcat(buffer, " ", buffersize);
				Q_strlcat(buffer, type->params[i].paramname, buffersize);
			}

			if (++i < type->num_parms || vargs)
				Q_strlcat(buffer, ", ", buffersize);
		}
		if (vargs)
			Q_strlcat(buffer, "...", buffersize);
		Q_strlcat(buffer, ")", buffersize);
	}
	else if (type->type == ev_entity && type->parentclass)
	{
		ret = buffer;
		*ret = 0;
		Q_strlcat(buffer, "class ", buffersize);
		Q_strlcat(buffer, type->name, buffersize);
/*		strcat(ret, " {");
		type = type->param;
		while(type)
		{
			strcat(ret, type->name);
			type = type->next;

			if (type)
				strcat(ret, ", ");
		}
		strcat(ret, "}");
*/
	}
	else
		Q_strlcat(buffer, type->name, buffersize);

	return buffer;
}
//#define typecmp(a, b) (a && ((a)->type==(b)->type) && !STRCMP((a)->name, (b)->name))

QCC_type_t *QCC_PR_FindType (QCC_type_t *type)
{
	int t;
	for (t = 0; t < numtypeinfos; t++)
	{
//		check = &qcc_typeinfo[t];
		if (typecmp_strict(&qcc_typeinfo[t], type))
			continue;

//		c2 = check->next;
//		n2 = type->next;
//		for (i=0 ; n2&&c2 ; i++)
//		{
//			if (!typecmp((c2), (n2)))
//				break;
//			c2=c2->next;
//			n2=n2->next;
//		}

//		if (n2==NULL&&c2==NULL)
		{
			return &qcc_typeinfo[t];
		}
	}
QCC_Error(ERR_INTERNAL, "Error with type");

	return type;
}
/*
QCC_type_t *QCC_PR_NextSubType(QCC_type_t *type, QCC_type_t *prev)
{
	int p;
	if (!prev)
		return type->next;

	for (p = prev->num_parms; p; p--)
		prev = QCC_PR_NextSubType(prev, NULL);
	if (prev->num_parms)

	switch(prev->type)
	{
	case ev_function:

	}

	return prev->next;
}
*/

QCC_type_t *QCC_TypeForName(char *name)
{
	int i;

	for (i = 0; i < numtypeinfos; i++)
	{
		if (qcc_typeinfo[i].typedefed && !STRCMP(qcc_typeinfo[i].name, name))
		{
			return &qcc_typeinfo[i];
		}
	}

	return NULL;
}

/*
============
PR_SkipToSemicolon

For error recovery, also pops out of nested braces
============
*/
void QCC_PR_SkipToSemicolon (void)
{
	do
	{
		if (!pr_bracelevel && QCC_PR_CheckToken (";"))
			return;
		QCC_PR_Lex ();
	} while (pr_token_type != tt_eof);
}


/*
============
PR_ParseType

Parses a variable type, including field and functions types
============
*/
#ifdef MAX_EXTRA_PARMS
char	pr_parm_names[MAX_PARMS+MAX_EXTRA_PARMS][MAX_NAME];
#else
char	pr_parm_names[MAX_PARMS][MAX_NAME];
#endif
char *pr_parm_argcount_name;

int recursivefunctiontype;

//expects a ( to have already been parsed.
QCC_type_t *QCC_PR_ParseFunctionType (int newtype, QCC_type_t *returntype)
{
	QCC_type_t	*ftype;
	char	*name;
	int definenames = !recursivefunctiontype;
	int optional = 0;
	int numparms = 0;
	struct QCC_typeparam_s paramlist[MAX_PARMS+MAX_EXTRA_PARMS];

	recursivefunctiontype++;

	ftype = QCC_PR_NewType(type_function->name, ev_function, false);

	ftype->aux_type = returntype;	// return type
	ftype->num_parms = 0;

	if (definenames)
		pr_parm_argcount_name = NULL;

	if (!QCC_PR_CheckToken (")"))
	{
		do
		{
			if (ftype->num_parms>=MAX_PARMS+MAX_EXTRA_PARMS)
				QCC_PR_ParseError(ERR_TOOMANYTOTALPARAMETERS, "Too many parameters. Sorry. (limit is %i)\n", MAX_PARMS+MAX_EXTRA_PARMS);

			if (QCC_PR_CheckToken ("..."))
			{
				ftype->vargs = true;
				break;
			}

			if (QCC_PR_CheckKeyword(keyword_optional, "optional"))
			{
				paramlist[numparms].optional = true;
				optional = true;
			}
			else
			{
				if (optional)
				{
					QCC_PR_ParseWarning(WARN_MISSINGOPTIONAL, "optional not specified on all optional args\n");
					paramlist[numparms].optional = true;
				}
				else
					paramlist[numparms].optional = false;
			}

			paramlist[numparms].ofs = 0;
			paramlist[numparms].arraysize = 0;
			paramlist[numparms].type = QCC_PR_ParseType(false, false);

			if (paramlist[numparms].type->type == ev_void)
				break;

//			type->name = "FUNC PARAMETER";

			paramlist[numparms].paramname = "";
			if (STRCMP(pr_token, ",") && STRCMP(pr_token, ")"))
			{
				if (QCC_PR_CheckToken ("..."))
				{
					ftype->vargs = true;
					break;
				}
				newtype = true;
				name = QCC_PR_ParseName ();
				paramlist[numparms].paramname = qccHunkAlloc(strlen(name)+1);
				strcpy(paramlist[numparms].paramname, name);
				if (definenames)
					strcpy (pr_parm_names[numparms], name);
			}
			else if (definenames)
				strcpy (pr_parm_names[numparms], "");
			numparms++;
		} while (QCC_PR_CheckToken (","));

		if (ftype->vargs)
		{
			if (!QCC_PR_CheckToken (")"))
			{
				name = QCC_PR_ParseName();
				if (definenames)
				{
					pr_parm_argcount_name = qccHunkAlloc(strlen(name)+1);
					strcpy(pr_parm_argcount_name, name);
				}
				ftype->vargcount = true;
				QCC_PR_Expect (")");
			}
		}
		else
			QCC_PR_Expect (")");
	}
	ftype->num_parms = numparms;
	ftype->params = qccHunkAlloc(sizeof(*ftype->params) * numparms);
	memcpy(ftype->params, paramlist, sizeof(*ftype->params) * numparms);
	recursivefunctiontype--;
	if (newtype)
		return ftype;
	return QCC_PR_FindType (ftype);
}
QCC_type_t *QCC_PR_ParseFunctionTypeReacc (int newtype, QCC_type_t *returntype)
{
	QCC_type_t	*ftype, *nptype;
	char	*name;
	int definenames = !recursivefunctiontype;
	int numparms = 0;
	struct QCC_typeparam_s paramlist[MAX_PARMS+MAX_EXTRA_PARMS];

	recursivefunctiontype++;

	ftype = QCC_PR_NewType(type_function->name, ev_function, false);

	ftype->aux_type = returntype;	// return type
	ftype->num_parms = 0;

	pr_parm_argcount_name = NULL;

	if (!QCC_PR_CheckToken (")"))
	{
		do
		{
			if (numparms>=MAX_PARMS+MAX_EXTRA_PARMS)
				QCC_PR_ParseError(ERR_TOOMANYTOTALPARAMETERS, "Too many parameters. Sorry. (limit is %i)\n", MAX_PARMS+MAX_EXTRA_PARMS);

			if (QCC_PR_CheckToken ("..."))
			{
				ftype->vargs = true;
				break;
			}

			if (QCC_PR_CheckName("arg"))
			{
				name = "";
				nptype = QCC_PR_NewType("Variant", ev_variant, false);
			}
			else if (QCC_PR_CheckName("vect"))	//this can only be of vector sizes, so...
			{
				name = "";
				nptype = QCC_PR_NewType("Vector", ev_vector, false);
			}
			else
			{
				name = QCC_PR_ParseName();
				QCC_PR_Expect(":");
				nptype = QCC_PR_ParseType(true, false);
			}

			if (nptype->type == ev_void)
				break;
//			type->name = "FUNC PARAMETER";


			paramlist[numparms].optional = false;
			paramlist[numparms].ofs = 0;
			paramlist[numparms].arraysize = 0;
			paramlist[numparms].type = nptype;

			if (!*name)
				paramlist[numparms].paramname = "";
			else
			{
				paramlist[numparms].paramname = qccHunkAlloc(strlen(name)+1);
				strcpy(paramlist[numparms].paramname, name);
			}
			if (definenames)
				QC_snprintfz(pr_parm_names[numparms], MAX_NAME, "%s", name);

			numparms++;
		} while (QCC_PR_CheckToken (";"));
		QCC_PR_Expect (")");
	}
	ftype->num_parms = numparms;
	ftype->params = qccHunkAlloc(sizeof(*ftype->params) * numparms);
	memcpy(ftype->params, paramlist, sizeof(*ftype->params) * numparms);
	recursivefunctiontype--;
	if (newtype)
		return ftype;
	return QCC_PR_FindType (ftype);
}
QCC_type_t *QCC_PR_PointerType (QCC_type_t *pointsto)
{
	QCC_type_t	*ptype, *e;
	char name[128];
	if (pointsto->ptrto)
		return pointsto->ptrto;
	QC_snprintfz(name, sizeof(name), "%s*", pointsto->name);
	ptype = QCC_PR_NewType(name, ev_pointer, false);
	ptype->aux_type = pointsto;
	e = QCC_PR_FindType (ptype);
	if (e == ptype)
	{
		char name[128];
		QC_snprintfz(name, sizeof(name), "ptr to %s", pointsto->name);
		e->name = qccHunkAlloc(strlen(name)+1);
		strcpy(e->name, name);
	}
	pointsto->ptrto = e;
	return e;
}
QCC_type_t *QCC_PR_FieldType (QCC_type_t *pointsto)
{
	QCC_type_t	*ptype;
	char name[128];
	QC_snprintfz(name, sizeof(name), "FIELD_TYPE(%s)", pointsto->name);
	ptype = QCC_PR_NewType(name, ev_field, false);
	ptype->aux_type = pointsto;
	ptype->size = ptype->aux_type->size;
	return QCC_PR_FindType (ptype);
}
QCC_type_t *QCC_PR_GenFunctionType (QCC_type_t *rettype, QCC_type_t **args, char **argnames, int numargs)
{
	int i;
	struct QCC_typeparam_s *p;
	QCC_type_t *ftype;
	ftype = QCC_PR_NewType("$func", ev_function, false);
	ftype->aux_type = rettype;
	ftype->num_parms = numargs;
	ftype->params = p = qccHunkAlloc(sizeof(*ftype->params)*numargs);
	ftype->vargs = false;
	ftype->vargcount = false;
	for (i = 0; i < numargs; i++, p++)
	{
		p->paramname = qccHunkAlloc(strlen(argnames[i])+1);
		strcpy(p->paramname, argnames[i]);
		p->type = args[i];
		p->optional = false;
		p->ofs = 0;
		p->arraysize = 0;
	}
	return QCC_PR_FindType (ftype);
}

extern char *basictypenames[];
extern QCC_type_t **basictypes[];
QCC_def_t *QCC_PR_DummyDef(QCC_type_t *type, char *name, QCC_function_t *scope, int arraysize, QCC_def_t *rootsymbol, unsigned int ofs, int referable, unsigned int flags);

void QCC_PR_ParseInitializerDef(QCC_def_t *def);

pbool type_inlinefunction;
/*newtype=true: creates a new type always
  silentfail=true: function is permitted to return NULL if it was not given a type, otherwise never returns NULL
*/
QCC_type_t *QCC_PR_ParseType (int newtype, pbool silentfail)
{
	QCC_type_t	*newparm;
	QCC_type_t	*newt;
	QCC_type_t	*type;
	char	*name;
	int i;
	etype_t structtype;

	type_inlinefunction = false;	//doesn't really matter so long as its not from an inline function type

//	int ofs;

	if (QCC_PR_CheckToken (".."))	//so we don't end up with the user specifying '. .vector blah' (hexen2 added the .. token for array ranges)
	{
		newt = QCC_PR_NewType("FIELD_TYPE", ev_field, false);
		newt->aux_type = QCC_PR_ParseType (false, false);

		newt->size = newt->aux_type->size;

		newt = QCC_PR_FindType (newt);

		type = QCC_PR_NewType("FIELD_TYPE", ev_field, false);
		type->aux_type = newt;

		type->size = type->aux_type->size;

		if (newtype)
			return type;
		return QCC_PR_FindType (type);
	}
	if (QCC_PR_CheckToken ("."))
	{
		newt = QCC_PR_NewType("FIELD_TYPE", ev_field, false);

		//.float *foo; is annoying.
		//technically it is a pointer to a .float
		//most people will want a .(float*) foo;
		//so .*float will give you that.
		//however, we can't cope parsing that with regular types, so we support that ONLY when . was already specified.
		//this is pretty much an evil syntax hack.
		if (QCC_PR_CheckToken ("*"))
		{
			newt->aux_type = QCC_PR_NewType("POINTER TYPE", ev_pointer, false);
			newt->aux_type->aux_type = QCC_PR_ParseType (false, false);

			newt->aux_type->size = newt->aux_type->aux_type->size;
		}
		else
			newt->aux_type = QCC_PR_ParseType (false, false);

		newt->size = newt->aux_type->size;

		if (newtype)
			return newt;
		return QCC_PR_FindType (newt);
	}

	name = QCC_PR_CheckCompConstString(pr_token);

	//accessors
	if (QCC_PR_CheckKeyword (keyword_class, "accessor"))
	{
		char parentname[256];
		char *accessorname;
		char *funcname;

		newt = NULL;

		funcname = QCC_PR_ParseName();
		accessorname = qccHunkAlloc(strlen(funcname)+1);
		strcpy(accessorname, funcname);

		/* Look to see if this type is already defined */
		for(i=0;i<numtypeinfos;i++)
		{
			if (!qcc_typeinfo[i].typedefed)
				continue;
			if (STRCMP(qcc_typeinfo[i].name, accessorname) == 0 && qcc_typeinfo[i].type == ev_accessor)
			{
				newt = &qcc_typeinfo[i];
				break;
			}
		}

		if (QCC_PR_CheckToken(":"))
		{
			type = QCC_PR_ParseType(false, false);
			if (type)
				TypeName(type, parentname, sizeof(parentname));
			else
				strcpy(parentname, "??");

			if (!type || type->type == ev_struct || type->type == ev_union)
				QCC_PR_ParseError(ERR_NOTANAME, "Accessor %s cannot be based upon %s", accessorname, parentname);
		}
		else
			type = NULL;

		if (!newt)
		{
			newt = QCC_PR_NewType(accessorname, ev_accessor, true);
			newt->size=type->size;
		}
		if (!newt->parentclass)
		{
			newt->parentclass = type;
			if (!newt->parentclass || newt->parentclass->type == ev_struct || newt->parentclass->type == ev_union || newt->size != newt->parentclass->size)
				QCC_PR_ParseError(ERR_NOTANAME, "Accessor %s cannot be based upon %s", accessorname, parentname);
		}
		else if (type != newt->parentclass)
			QCC_PR_ParseError(ERR_NOTANAME, "Accessor %s basic type mismatch", accessorname);

		if (QCC_PR_CheckToken("{"))
		{
			struct accessor_s  *acc;
			pbool setnotget;
			char *fieldtypename;
			QCC_type_t *fieldtype;
			QCC_type_t *indextype;
			QCC_sref_t def;
			QCC_type_t *functype;
			char *argname[3];
			QCC_type_t *arg[3];
			int args;
			char *indexname;
			pbool isref;
			pbool isinline;


			do
			{
				isinline = QCC_PR_CheckName("inline");

				if (QCC_PR_CheckName("set"))
					setnotget = true;
				else if (QCC_PR_CheckName("get"))
					setnotget = false;
				else
					break;
				isref = QCC_PR_CheckToken("*");

				fieldtypename = QCC_PR_ParseName();
				fieldtype = QCC_TypeForName(fieldtypename);
				if (!fieldtype)
					QCC_PR_ParseError(ERR_NOTATYPE, "Invalid type: %s", fieldtypename);
				while(QCC_PR_CheckToken("*"))
					fieldtype = QCC_PR_PointerType(fieldtype);

				if (pr_token_type != tt_punct)
				{
					funcname = QCC_PR_ParseName();
					accessorname = qccHunkAlloc(strlen(funcname)+1);
					strcpy(accessorname, funcname);
				}
				else
					accessorname = "";

				indextype = NULL;
				indexname = "index";
				if (QCC_PR_CheckToken("["))
				{
					fieldtypename = QCC_PR_ParseName();
					indextype = QCC_TypeForName(fieldtypename);

					if (!QCC_PR_CheckToken("]"))
					{
						indexname = QCC_PR_ParseName();
						QCC_PR_Expect("]");
					}
				}

				QCC_PR_Expect("=");

				args = 0;
				strcpy (pr_parm_names[args], "this");
				argname[args] = "this";
				if (isref)
					arg[args++] = QCC_PointerTypeTo(newt);
				else
					arg[args++] = newt;
				if (indextype)
				{
					strcpy (pr_parm_names[args], indexname);
					argname[args] = indexname;
					arg[args++] = indextype;
				}
				if (setnotget)
				{
					strcpy (pr_parm_names[args], "value");
					argname[args] = "value";
					arg[args++] = fieldtype;
				}
				functype = QCC_PR_GenFunctionType(setnotget?type_void:fieldtype, arg, argname, args);

				if (pr_token_type != tt_name)
				{
					QCC_function_t *f;
					char funcname[256];
					QC_snprintfz(funcname, sizeof(funcname), "%s::%s_%s", newt->name, setnotget?"set":"get", accessorname);

					def = QCC_PR_GetSRef(functype, funcname, NULL, true, 0, GDF_CONST | (isinline?GDF_INLINE:0));

					//pr_classtype = newt;
					f = QCC_PR_ParseImmediateStatements (def.sym, functype);
					pr_classtype = NULL;
					pr_scope = NULL;
					def.sym->symboldata[def.ofs].function = f - functions;
					f->def = def.sym;
					def.sym->initialized = 1;
				}
				else
				{
					funcname = QCC_PR_ParseName();
					def = QCC_PR_GetSRef(functype, funcname, NULL, true, 0, GDF_CONST|(isinline?GDF_INLINE:0));
					if (!def.cast)
						QCC_Error(ERR_NOFUNC, "%s::set_%s: %s was not defined", newt->name, accessorname, funcname);
				}
				if (!def.cast || !def.sym || def.sym->temp)
					QCC_Error(ERR_NOFUNC, "%s::%s_%s function invalid", newt->name, setnotget?"set":"get", accessorname);
				
				for (acc = newt->accessors; acc; acc = acc->next)
					if (!strcmp(acc->fieldname, accessorname))
						break;
				if (!acc)
				{
					acc = qccHunkAlloc(sizeof(*acc));
					acc->fieldname = accessorname;
					acc->next = newt->accessors;
					acc->type = fieldtype;
					acc->indexertype = indextype;
					newt->accessors = acc;
				}

				if (acc->getset_func[setnotget].cast)
					QCC_Error(ERR_TOOMANYINITIALISERS, "%s::%s_%s already declared", newt->name, setnotget?"set":"get", accessorname);
				acc->getset_func[setnotget] = def;
				acc->getset_isref[setnotget] = isref;
				QCC_FreeTemp(def);
			} while (QCC_PR_CheckToken(",") || QCC_PR_CheckToken(";"));
			QCC_PR_Expect("}");
		}

		if (newtype)
			newt = QCC_PR_DuplicateType(newt, false);
		return newt;
	}

	if (QCC_PR_CheckKeyword (keyword_class, "class"))
	{
//		int parms;
		QCC_type_t *fieldtype;
		char membername[2048];
		char *classname;
		int forwarddeclaration;
		int numparms = 0;
		struct QCC_typeparam_s *parms = NULL;
		char *parmname;
		int arraysize;
		pbool redeclaration;
		int basicindex;
		QCC_def_t *d;
		QCC_type_t *pc;
		pbool found = false;
		int assumevirtual = 0;	//0=erk, 1=yes, -1=no

		parmname = QCC_PR_ParseName();
		classname = qccHunkAlloc(strlen(parmname)+1);
		strcpy(classname, parmname);

		newt = 0;

		if (QCC_PR_CheckToken(":"))
		{
			char *parentname = QCC_PR_ParseName();
			fieldtype = QCC_TypeForName(parentname);
			if (!fieldtype)
				QCC_PR_ParseError(ERR_NOTANAME, "Parent class %s was not yet defined", parentname);
			forwarddeclaration = false;

			QCC_PR_Expect("{");
		}
		else
		{
			fieldtype = type_entity;
			forwarddeclaration = !QCC_PR_CheckToken("{");
		}

		/* Look to see if this type is already defined */
		for(i=0;i<numtypeinfos;i++)
		{
			if (!qcc_typeinfo[i].typedefed)
				continue;
			if (STRCMP(qcc_typeinfo[i].name, classname) == 0)
			{
				newt = &qcc_typeinfo[i];
				break;
			}
		}

		if (newt && newt->num_parms != 0)
			redeclaration = true;
		else
			redeclaration = false;

		if (!newt)
		{
			newt = QCC_PR_NewType(classname, ev_entity, true);
			newt->size=type_entity->size;
		}

		type = NULL;

		if (forwarddeclaration)
			return newt;

		if (pr_scope)
			QCC_PR_ParseError(ERR_REDECLARATION, "Declaration of class %s within function", classname);
		if (redeclaration && fieldtype != newt->parentclass)
			QCC_PR_ParseError(ERR_REDECLARATION, "Parent class changed on redeclaration of %s", classname);
		newt->parentclass = fieldtype;

		if (QCC_PR_CheckToken(","))
			QCC_PR_ParseError(ERR_NOTANAME, "member missing name");
		while (!QCC_PR_CheckToken("}"))
		{
			pbool havebody = false;
			pbool isnull = false;
			pbool isvirt = false;
			pbool isnonvirt = false;
			pbool isstatic = false;
			pbool isignored = false;
			pbool ispublic = false;
			pbool isprivate = false;
			pbool isprotected = false;
			while(1)
			{
				if (QCC_PR_CheckKeyword(1, "nonvirtual"))
					isnonvirt = true;
				else if (QCC_PR_CheckKeyword(1, "static"))
					isstatic = true;
				else if (QCC_PR_CheckKeyword(1, "virtual"))
					isvirt = true;
				else if (QCC_PR_CheckKeyword(1, "ignore"))
					isignored = true;
				else if (QCC_PR_CheckKeyword(1, "strip"))
					isignored = true;
				else if (QCC_PR_CheckKeyword(1, "public"))
					ispublic = true;
				else if (QCC_PR_CheckKeyword(1, "private"))
					isprivate = true;
				else if (QCC_PR_CheckKeyword(1, "protected"))
					isprotected = true;
				else
					break;
			}
			if (QCC_PR_CheckToken(":"))
			{
				if (isvirt && !isnonvirt)
					assumevirtual = 1;
				else if (isnonvirt && !isvirt)
					assumevirtual = -1;
				continue;
			}
			newparm = QCC_PR_ParseType(false, false);

			if (!newparm)
				QCC_PR_ParseError(ERR_INTERNAL, "In class %s, expected type, found %s", classname, pr_token);

			if (newparm->type == ev_struct || newparm->type == ev_union)	//we wouldn't be able to handle it.
				QCC_PR_ParseError(ERR_INTERNAL, "Struct or union in class %s", classname);

			parmname = QCC_PR_ParseName();
			if (QCC_PR_CheckToken("["))
			{
				arraysize = QCC_PR_IntConstExpr();
				QCC_PR_Expect("]");
			}
			else
				arraysize = 0;

			if (newparm->type == ev_function)
			{
				if (isstatic)
				{
					isstatic = false;
					isnonvirt = true;
//					QCC_PR_ParseError(ERR_INTERNAL, "%s::%s static member functions are not supported at this time.", classname, parmname);
				}

				if (!strcmp(classname, parmname))
				{
					if (isstatic)
						QCC_PR_ParseError(ERR_INTERNAL, "Constructor %s::%s may not be static.", classname, pr_token);
					if (!isvirt)
						isnonvirt = true;//silently promote constructors to static
				}
				else if (!isvirt && !isnonvirt && !isstatic)
				{
					if (assumevirtual == 1)
						isvirt = true;
					else if (assumevirtual == -1)
						isnonvirt = true;
					else
						QCC_PR_ParseWarning(WARN_MISSINGMEMBERQUALIFIER, "%s::%s was not qualified. Assuming non-virtual.", classname, parmname);
				}
				if (isvirt+isnonvirt+isstatic != 1)
					QCC_PR_ParseError(ERR_INTERNAL, "Multiple conflicting qualifiers on %s::%s.", classname, pr_token);
			}
			else
			{
				if (isvirt|isnonvirt)
					QCC_Error(ERR_INTERNAL, "virtual keyword on member that is not a function");
			}

			if (newparm->type == ev_function)
			{
				if (QCC_PR_CheckToken("="))
				{
					havebody = true;
				}
				else if (pr_token[0] == '{')
					havebody = true;
			}

			if (havebody)
			{
				QCC_def_t *def;
				if (pr_scope)
					QCC_Error(ERR_INTERNAL, "Nested function declaration");

				isnull = (QCC_PR_CheckImmediate("0") || QCC_PR_CheckImmediate("0i"));
				QC_snprintfz(membername, sizeof(membername), "%s::%s", classname, parmname);
				if (isnull)
				{
					if (isignored)
						def = NULL;
					else
					{
						def = QCC_PR_GetDef(newparm, membername, NULL, true, 0, 0);
						def->symboldata[def->ofs].function = 0;
						def->initialized = 1;
					}
				}
				else
				{
					if (isignored)
						def = NULL;
					else
						def = QCC_PR_GetDef(newparm, membername, NULL, true, 0, GDF_CONST);

					if (newparm->type != ev_function)
						QCC_Error(ERR_INTERNAL, "Can only initialise member functions");
					else
					{
						if (autoprototype || isignored)
						{
							if (QCC_PR_CheckToken("["))
							{
								while (!QCC_PR_CheckToken("]"))
								{
									if (pr_token_type == tt_eof)
										break;
									QCC_PR_Lex();
								}
							}
							QCC_PR_Expect("{");

							{
								int blev = 1;
								//balance out the { and }
								while(blev)
								{
									if (pr_token_type == tt_eof)
										break;
									if (QCC_PR_CheckToken("{"))
										blev++;
									else if (QCC_PR_CheckToken("}"))
										blev--;
									else
										QCC_PR_Lex();	//ignore it.
								}
							}
						}
						else
						{
							pr_classtype = newt;
							QCC_PR_ParseInitializerDef(def);
							QCC_FreeDef(def);
							pr_classtype = NULL;
							/*
							f = QCC_PR_ParseImmediateStatements (def, newparm);
							pr_classtype = NULL;
							pr_scope = NULL;
							def->symboldata[def->ofs].function = f - functions;
							f->def = def;
							def->initialized = 1;*/
						}
					}
				}
				if (def)
					QCC_FreeDef(def);

				if (!isvirt && !isignored)
				{
					QCC_def_t *fdef;
					QCC_type_t *pc;
					unsigned int i;

					for (pc = newt->parentclass; pc; pc = pc->parentclass)
					{
						for (i = 0; i < pc->num_parms; i++)
						{
							if (!strcmp(pc->params[i].paramname, parmname))
							{
								QCC_PR_ParseWarning(WARN_DUPLICATEDEFINITION, "%s::%s is virtual inside parent class '%s'. Did you forget the 'virtual' keyword?", newt->name, parmname, pc->name);
								break;
							}
						}
						if (i < pc->num_parms)
							break;
					}
					if (!pc)
					{
						fdef = QCC_PR_GetDef(NULL, parmname, NULL, false, 0, GDF_CONST);
						if (fdef && fdef->type->type == ev_field)
						{
							QCC_PR_ParseWarning(WARN_DUPLICATEDEFINITION, "%s::%s is virtual inside parent class 'entity'. Did you forget the 'virtual' keyword?", newt->name, parmname);
						}
					}
				}
			}

			QCC_PR_Expect(";");

			if (isignored)	//member doesn't really exist
				continue;

			//static members are technically just funny-named globals, and do not generate fields.
			if (isnonvirt || isstatic || (newparm->type == ev_function && !arraysize))
			{
				QC_snprintfz(membername, sizeof(membername), "%s::%s", classname, parmname);
				QCC_FreeDef(QCC_PR_GetDef(newparm, membername, NULL, true, 0, GDF_CONST));

				if (isnonvirt || isstatic)
					continue;
			}

			
			fieldtype = QCC_PR_NewType(parmname, ev_field, false);
			fieldtype->aux_type = newparm;
			fieldtype->size = newparm->size;

			parms = realloc(parms, sizeof(*parms) * (numparms+1));
			parms[numparms].ofs = 0;
			parms[numparms].optional = false;
			parms[numparms].paramname = parmname;
			parms[numparms].arraysize = arraysize;
			parms[numparms].type = newparm;

			basicindex = 0;
			found = false;
			for(pc = newt; pc && !found; pc = pc->parentclass)
			{
				struct QCC_typeparam_s *pp;
				int numpc;
				int i;
				if (pc == newt)
				{
					pp = parms;
					numpc = numparms;
				}
				else
				{
					pp = pc->params;
					numpc = pc->num_parms;
				}
				for (i = 0; i < numpc; i++)
				{
					if (pp[i].type->type == newparm->type)
					{
						if (!strcmp(pp[i].paramname, parmname))
						{
							if (typecmp(pp[i].type, newparm))
							{
								char bufc[256];
								char bufp[256];
								TypeName(pp[i].type, bufp, sizeof(bufp));
								TypeName(newparm, bufc, sizeof(bufc));
								QCC_PR_ParseError(0, "%s defined as %s in %s, but %s in %s\n", parmname, bufc, newt->name, bufp, pc->name);
							}
							basicindex = pp[i].ofs;
							found = true;
							break;
						}
						if ((unsigned int)basicindex < pp[i].ofs+1)	//if we found one with the index
							basicindex = pp[i].ofs+1;	//make sure we don't union it.
					}
				}
			}
			parms[numparms].ofs = basicindex;	//ulp, its new
			numparms++;

			if (found)
				continue;

			if (!*basictypes[newparm->type])
				QCC_PR_ParseError(0, "members of type %s are not supported (%s::%s)\n", basictypenames[newparm->type], classname, parmname);

			//make sure the union is okay
			d = QCC_PR_GetDef(NULL, parmname, NULL, 0, 0, GDF_CONST);
			if (!d)
			{	//don't go all weird with unioning generic fields
				QC_snprintfz(membername, sizeof(membername), "::%s%i", basictypenames[newparm->type], basicindex+1);
				d = QCC_PR_GetDef(NULL, membername, NULL, 0, 0, GDF_CONST);
				if (!d)
				{
					d = QCC_PR_GetDef(QCC_PR_FieldType(*basictypes[newparm->type]), membername, NULL, 2, 0, GDF_CONST);
					for (i = 0; (unsigned int)i < newparm->size; i++)
						d->symboldata[i]._int = pr.size_fields+i;
					pr.size_fields += i;

					d->referenced = true;	//always referenced, so you can inherit safely.
				}
			}
			QCC_FreeDef(d);

			//and make sure we can do member::__fname
			//actually, that seems pointless.
			QC_snprintfz(membername, sizeof(membername), "%s::"MEMBERFIELDNAME, classname, parmname);
//			printf("define %s -> %s\n", membername, d->name);
			d = QCC_PR_DummyDef(fieldtype, membername, pr_scope, 0, d, 0, true, (isnull?0:GDF_CONST)|(opt_classfields?GDF_STRIP:0));
			d->referenced = true;	//always referenced, so you can inherit safely.
		}

		if (redeclaration)
		{
			int i;
			redeclaration = newt->num_parms != numparms;

			for (i = 0; i < numparms && (unsigned int)i < newt->num_parms; i++)
			{
				if (newt->params[i].arraysize != parms[i].arraysize || typecmp(newt->params[i].type, parms[i].type) || strcmp(newt->params[i].paramname, parms[i].paramname))
				{
					QCC_PR_ParseError(ERR_REDECLARATION, "Incompatible redeclaration of class %s. %s differs.", classname, parms[i].paramname);
					break;
				}
			}
			if (newt->num_parms != numparms)
				QCC_PR_ParseError(ERR_REDECLARATION, "Incompatible redeclaration of class %s.", classname);
		}
		else
		{
			newt->num_parms = numparms;
			newt->params = qccHunkAlloc(sizeof(*type->params) * numparms);
			memcpy(newt->params, parms, sizeof(*type->params) * numparms);
		}
		free(parms);

		{
			QCC_def_t *d;
			//if there's a constructor, make sure the spawnfunc_ function is defined so that its available to maps.
			QC_snprintfz(membername, sizeof(membername), "spawnfunc_%s", classname);
			d = QCC_PR_GetDef(type_function, membername, NULL, true, 0, GDF_CONST);
			d->funccalled = true;
			d->referenced = true;
			QCC_FreeDef(d);
		}

		QCC_PR_Expect(";");
		return NULL;
	}

	structtype = ev_void;
	if (QCC_PR_CheckKeyword (keyword_union, "union"))
		structtype = ev_union;
	else if (QCC_PR_CheckKeyword (keyword_struct, "struct"))
		structtype = ev_struct;
	if (structtype != ev_void)
	{
		struct QCC_typeparam_s *parms = NULL;
		int numparms = 0;
		unsigned int arraysize;
		char *parmname;

		if (QCC_PR_CheckToken("{"))
		{
			//nameless struct
			newt = QCC_PR_NewType(structtype==ev_union?"<union>":"<struct>", structtype, false);
		}
		else
		{
			newt = QCC_TypeForName(pr_token);
			if (!newt)
				newt = QCC_PR_NewType(QCC_CopyString(pr_token)+strings, ev_struct, true);
			QCC_PR_Lex();
			if (newt->size)
			{
				if (QCC_PR_CheckToken("{"))
					QCC_PR_ParseError(ERR_NOTANAME, "%s %s is already defined", structtype==ev_union?"union":"struct", newt->name);
				return newt;
			}

			//struct declaration only, not definition.
			if (!QCC_PR_CheckToken("{"))
				return newt;
		}
		newt->size=0;

		type = NULL;
		if (QCC_PR_CheckToken(","))
			QCC_PR_ParseError(ERR_NOTANAME, "element missing name");

		newparm = NULL;
		while (!QCC_PR_CheckToken("}"))
		{
			if (QCC_PR_CheckToken(","))
			{
				if (!newparm)
					QCC_PR_ParseError(ERR_NOTANAME, "element missing type");
				newparm = QCC_PR_NewType(newparm->name, newparm->type, false);
			}
			else
				newparm = QCC_PR_ParseType(false, false);

			arraysize = 0;

			if (!QCC_PR_CheckToken(";"))
			{
				parmname = qccHunkAlloc(strlen(pr_token)+1);
				strcpy(parmname, pr_token);
				QCC_PR_Lex();
				if (QCC_PR_CheckToken("["))
				{
					arraysize=QCC_PR_IntConstExpr();
					if (!arraysize)
						QCC_PR_ParseError(ERR_NOTANAME, "cannot cope with 0-sized arrays");
					QCC_PR_Expect("]");
				}
				QCC_PR_CheckToken(";");
			}
			else
				parmname = "";


			parms = realloc(parms, sizeof(*parms) * (numparms+1));
			if (structtype == ev_union)
			{
				parms[numparms].ofs = 0;
				if (newparm->size*(arraysize?arraysize:1) > newt->size)
					newt->size = newparm->size*(arraysize?arraysize:1);
			}
			else
			{
				parms[numparms].ofs = newt->size;
				newt->size += newparm->size*(arraysize?arraysize:1);
			}
			parms[numparms].arraysize = arraysize;
			parms[numparms].optional = false;
			parms[numparms].paramname = parmname;
			parms[numparms].type = newparm;
			numparms++;
		}
		if (!numparms)
			QCC_PR_ParseError(ERR_NOTANAME, "%s %s has no members", structtype==ev_union?"union":"struct", newt->name);

		newt->num_parms = numparms;
		newt->params = qccHunkAlloc(sizeof(*type->params) * numparms);
		memcpy(newt->params, parms, sizeof(*type->params) * numparms);
		free(parms);

		return newt;
	}

	type = NULL;
	for (i = 0; i < numtypeinfos; i++)
	{
		if (!qcc_typeinfo[i].typedefed)
			continue;
		if (!STRCMP(qcc_typeinfo[i].name, name))
		{
			type = &qcc_typeinfo[i];
			break;
		}
	}

	if (i == numtypeinfos)
	{
		if (!*name)
			return NULL;

		//some reacc types...
		if (!stricmp("Void", name))
			type = type_void;
		else if (!stricmp("Real", name))
			type = type_float;
		else if (!stricmp("Vector", name))
			type = type_vector;
		else if (!stricmp("Object", name))
			type = type_entity;
		else if (!stricmp("String", name))
			type = type_string;
		else if (!stricmp("PFunc", name))
			type = type_function;
		else
		{
			if (silentfail)
				return NULL;

			QCC_PR_ParseError (ERR_NOTATYPE, "\"%s\" is not a type", name);
			type = type_float;	// shut up compiler warning
		}
	}
	QCC_PR_Lex ();

	while (QCC_PR_CheckToken("*"))
		type = QCC_PointerTypeTo(type);

	if (QCC_PR_CheckToken ("("))	//this is followed by parameters. Must be a function.
	{
		type_inlinefunction = true;
		type = QCC_PR_ParseFunctionType(newtype, type);
	}
	else
	{
		if (newtype)
		{
			type = QCC_PR_DuplicateType(type, false);
		}
	}
	return type;
}

#endif



