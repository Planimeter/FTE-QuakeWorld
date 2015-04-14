void GoToDefinition(char *name);
int Grep(char *filename, char *string);
void EditFile(char *name, int line, pbool setcontrol);

void GUI_SetDefaultOpts(void);
int GUI_BuildParms(char *args, char **argv, pbool quick);

unsigned char *PDECL QCC_ReadFile (const char *fname, void *buffer, int len, size_t *sz);
int QCC_RawFileSize (const char *fname);
pbool QCC_WriteFile (const char *name, void *data, int len);
void GUI_DialogPrint(char *title, char *text);

extern char parameters[16384];

extern char progssrcname[256];
extern char progssrcdir[256];

extern pbool fl_nondfltopts;
extern pbool fl_hexen2;
extern pbool fl_ftetarg;
extern pbool fl_autohighlight;
extern pbool fl_compileonstart;
extern pbool fl_showall;
extern pbool fl_log;
