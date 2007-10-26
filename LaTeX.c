/*
 * LaTeX.c
 * pidgin-latex plugin
 *
 * This a plugin for Pidgin to display LaTeX formula in conversation
 *
 * PLEASE, send any comment, bug report, etc. to the trackers at sourceforge.net
 *
 * Copyright (C) 2006-2007 Benjamin Moll (qjuh@users.sourceforge.net)
 * some portions : Copyright (C) 2004-2006 Nicolas Schoonbroodt (nicolas@ffsa.be)
 *                 Copyright (C) 2004-2006 GRIm@ (thegrima@altern.org).
 * 		   Copyright (C) 2004-2006 Eric Betts (bettse@onid.orst.edu).
 * Windows port  : Copyright (C) 2005-2006 Nicolai Stange (nic-stange@t-online.de)
 * Other portions heavily inspired and copied from gaim sources
 * Copyright (C) 1998-2007 Pidgin developers pidgin.im
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; This document is under the scope of
 * the version 2 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see COPYING); if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "LaTeX.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef MAX
#define MAX(a,b) ( (a) > (b) ? (a ): (b))
#endif

#ifndef DWORD
#define DWORD unsigned long
#endif

#ifdef _WIN32
#include <windows.h>
#endif

static void
open_log(PurpleConversation *conv)
{
	conv->logs = g_list_append(NULL, purple_log_new(conv->type == PURPLE_CONV_TYPE_CHAT ? PURPLE_LOG_CHAT :
							   PURPLE_LOG_IM, conv->name, conv->account,
							   conv, time(NULL), NULL));
}

#ifdef _WIN32
void win32_purple_notify_error(char *prep)
{
  char *errmsg=NULL;
  char *finalmsg=NULL;
  if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (void*)&errmsg, 0, NULL))
  {
      purple_notify_error(NULL, "LaTeX" ,"Can't display error message.", NULL);
      return;
  }

  if(prep)
  {
    finalmsg=malloc((strlen(errmsg)+ strlen(prep) + 3)*sizeof(char));
    if(!finalmsg)
    {
      purple_notify_error(NULL, "LaTeX", "Can't display error message.", NULL);
      LocalFree(errmsg); /* we can't do anything more for you */
      return;
    }
    strcpy(finalmsg, prep);
    strcat(finalmsg, ": ");
    strcat(finalmsg, errmsg);
    LocalFree(errmsg);
  }
  else
  {
    finalmsg = malloc((strlen(errmsg)+1)*sizeof(char));
    if(!finalmsg)
    {
      purple_notify_error(NULL, "LaTeX", "Can't display error message.", NULL);
      LocalFree(errmsg); /* we can't do anything more for you */
      return;
    }
    strcpy(finalmsg, errmsg);
    LocalFree(errmsg);
  }
  purple_notify_error(NULL, "LaTeX", finalmsg, NULL);
  free(finalmsg);
  return;
}
#endif

static char* getdirname(const char const *file)
{
  char *s=NULL;
  char *r=NULL;
  s=strrchr(file, G_DIR_SEPARATOR);
  if(!s) /* just a pure filename without dir? */
  {
    /* Here is no standard-, but GNU-bahaviour of getcwd assumed.
       Note that msdn.microsoft.com defines the same as GNU.
     */
    return getcwd(NULL,0);
  }

  s+=1; /* get the G_DIR_SEPARATOR at the end of directory-string */
  r=malloc(s-file+sizeof(char));
  if(r)
  {
    memcpy(r,file, s-file);
    r[(s-file)/sizeof(char)]='\0';
  }
  return r;
}

static char* getfilename(const char const *file)
{
  char *s=NULL;
  char *r=NULL;
  s=strrchr(file,G_DIR_SEPARATOR);
  if(!s)
  {
    r=malloc((strlen(file)+1)*sizeof(char));
    strcpy(r,file);
    return r;
  }

  s+=1;
  r=malloc((strlen(file)+1)*sizeof(char)+file-s);
  if(r)
  {
    memcpy(r,s,strlen(file)*sizeof(char)+file-s);
    r[strlen(file)+(file-s)/sizeof(char)]='\0';
  }
  return r;
}

char* searchPATH(const char const *file)
{
  char *cmd=NULL;
#ifdef _WIN32
  DWORD sz=0;
  DWORD sz2=0;

  sz=SearchPath(NULL, file, ".exe", 0, cmd, NULL);
  cmd = malloc((sz+1)*sizeof(TCHAR));
  if(cmd)
  {
    sz2=SearchPath(NULL, file, ".exe", sz+1, cmd, NULL);
    if(!sz2)
    {
      free(cmd);
      cmd=NULL;
    }
  }
#else
  cmd=malloc((strlen(file)+1)*sizeof(char));
  if(cmd)
    strcpy(cmd, file);
#endif

  return cmd;
}

#ifdef _WIN32 /* we could take system(), too, but this opens ugly console-windows within IM-Session*/
static DWORD execute(char *cmd, char *opts[], int copts)
{
  int i=0;
  int len=strlen(cmd) + 4;
  char *params=NULL;
  DWORD exitcode=0;
  for(i=0; i<copts; ++i)
    len+=(strlen(opts[i]))*sizeof(char);

  params=malloc(len);
  if(!params)
    return -1;

  strcpy(params, "\"");
  strcat(params, cmd);
  strcat(params, "\" ");

  for(i=0; i<copts; ++i)
    strcat(params, opts[i]);

  STARTUPINFO sup;
  PROCESS_INFORMATION pi;
  ZeroMemory( &sup, sizeof(sup) );
  sup.cb = sizeof(sup);
  ZeroMemory( &pi, sizeof(pi) );
  sup.wShowWindow = SW_HIDE;
  sup.dwFlags = STARTF_USESHOWWINDOW;

  if(!CreateProcess(NULL, params, NULL, NULL, TRUE, 0, NULL, NULL, &sup, &pi))
  {
    win32_purple_notify_error(cmd);
    free(params);
    return -1;
  }

  free(params);
  if(WAIT_OBJECT_0!=WaitForSingleObjectEx(pi.hProcess, INFINITE, FALSE))
  {
    win32_purple_notify_error(cmd);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return -1;
  }

  do {
    if(!GetExitCodeThread(pi.hThread, &exitcode))
    {
      win32_purple_notify_error(cmd);
      return -1;
    }
    Sleep(10);
  } while(exitcode==STILL_ACTIVE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return exitcode;
}
#else /* not windows, take system() */
static int execute(char *cmd, char *opts[], int copts)
{
  int i=0;
  int exitcode=-1;
  int len=strlen(cmd) + 4;
  char *params=NULL;

  for(i=0; i<copts; ++i)
    len+=(strlen(opts[i]))*sizeof(char);

  params=malloc(len);
  if(!params)
    return -1;

  strcpy(params, "\"");
  strcat(params, cmd);
  strcat(params, "\" ");

  for(i=0; i<copts; ++i)
    strcat(params, opts[i]);

  exitcode=system(params);
  free(params);
  return exitcode;
}
#endif

static char* get_latex_cmd()
{
  return searchPATH("latex");
}

static char* get_dvips_cmd()
{
  return searchPATH("dvips");
}

static char* get_convert_cmd()
{
#ifdef _WIN32
    /*open registry*/
    DWORD type;
    DWORD cbData;
    char *pData=NULL;
    HKEY hk=NULL;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\ImageMagick\\Current",0,KEY_READ,&hk);
    RegQueryValueEx(hk, "BinPath", NULL, &type, NULL, &cbData);
    cbData=cbData + (1+12)*sizeof(char);
    pData=malloc(cbData);
    if(pData && RegQueryValueEx(hk, "BinPath", NULL, &type, pData, &cbData)==ERROR_SUCCESS)
    {
      CloseHandle(hk);
      strcat(pData,"\\convert.exe");
      return pData;
    }

    CloseHandle(hk);
    free(pData);
    return searchPATH("convert");
#else
  return searchPATH("convert");
#endif
}

static gboolean is_blacklisted(char *message)
{
  char *not_secure[NB_BLACKLIST] = BLACKLIST;
  int i;
  for (i = 0 ; i < NB_BLACKLIST ; i++)
    {
      char *begin_not_secure = malloc((strlen(not_secure[i])+9)*sizeof(char));
      strcpy(begin_not_secure,"\\begin{");
      strcat(begin_not_secure,not_secure[i]+0x01);
      strcat(begin_not_secure,"}");
      if (strstr(message, not_secure[i]) != NULL || strstr(message, begin_not_secure)) return TRUE;
    }
  return FALSE;
}

static gboolean latex_to_image(char *latex, char **file_tex, char **file_dvi, char **file_ps, char **file_png)
{
  FILE * texfile;
  char *file_tmp = NULL;
  char *tmpdir = NULL;
  char *cmdlatex = NULL;
  char *cmddvips = NULL;
  char *cmdconvert = NULL;

  /* the following is new and related to temporary-filename-generation */
  texfile = purple_mkstemp(&file_tmp,TRUE);
  *file_tex=malloc((strlen(file_tmp)+5)*sizeof(char));
  *file_dvi=malloc((strlen(file_tmp)+5)*sizeof(char));
  *file_ps=malloc((strlen(file_tmp)+4)*sizeof(char));
  *file_png=malloc((strlen(file_tmp)+5)*sizeof(char));
  if(!(file_tmp && *file_tex && *file_dvi && *file_ps && *file_png))
  {
    fclose(texfile);
    unlink(file_tmp);
    free(file_tmp);
    free(*file_tex);
    free(*file_dvi);
    free(*file_ps);
    free(*file_png);
    *file_tex=*file_dvi=*file_ps=*file_png=NULL;
    return FALSE;
  }
  strcpy(*file_tex, file_tmp);
  strcat(*file_tex, ".tex");
  strcpy(*file_dvi, file_tmp);
  strcat(*file_dvi, ".dvi");
  strcpy(*file_ps, file_tmp);
  strcat(*file_ps, ".ps");
  strcpy(*file_png, file_tmp);
  strcat(*file_png, ".jpg");
  free(file_tmp);
  fclose(texfile);

  if (! (texfile = fopen(*file_tex, "w"))) return FALSE;
  fprintf(texfile, HEADER HEADER_MATH "%s" FOOTER_MATH FOOTER, latex);
  fclose (texfile);

  tmpdir=getdirname(*file_tex);
  /* generate commands, also new */
  char *latexopts[5]={"--interaction=nonstopmode", " ", "\"", *file_tex, "\""};
  char *dvipsopts[8]={"-E", " ", "-o", " \"", *file_ps, "\" \"", *file_dvi, "\""};
  char *convertopts[5]={"\"", *file_ps, "\" \"", *file_png, "\""};
  cmdlatex=get_latex_cmd();
  cmddvips=get_dvips_cmd();
  cmdconvert=get_convert_cmd();

  if (!tmpdir || chdir(tmpdir) || !cmdlatex || !cmddvips || !cmdconvert)
  {
    free(*file_dvi);
    free(*file_ps);
    free(*file_png);
    *file_dvi=*file_ps=*file_png=NULL;
    free(cmdlatex);
    free(cmddvips);
    free(cmdconvert);
    return FALSE;
  }

  free(tmpdir);
  if((execute(cmdlatex, latexopts, 5) || execute(cmddvips, dvipsopts, 8) ||  execute(cmdconvert, convertopts, 5))) return FALSE;

  free(cmdlatex);
  free(cmddvips);
  free(cmdconvert);

  return TRUE;
}

static gboolean analyse(PurpleConversation *conv, char **tmp2, char *startdelim, char *enddelim, gboolean remote)
{
  int pos1, pos2, idimg;
  char *ptr1, *ptr2;
  char *file_tex=NULL;
  char *file_dvi=NULL;
  char *file_ps=NULL;
  char *file_png=NULL;

  ptr1 = strstr(*tmp2, startdelim);
  while(ptr1 != NULL)
    {
      char *tex, *message, *filter, *idstring;
      gchar *name, *buf, *filedata;
      size_t size;
      GError *error = NULL;

      pos1 = strlen(*tmp2) - strlen(ptr1);

      // Have to ignore the first 2 char ("$$") --> & [2]
      ptr2 = strstr(&ptr1[strlen(startdelim)], enddelim);
      if (ptr2 == NULL)
	{ return FALSE; }

      pos2 = strlen(*tmp2) - strlen(ptr2) + strlen(enddelim);

      if ((tex = malloc(pos2 - pos1 - strlen(enddelim) - strlen(startdelim) + 1)) == NULL)
	{
	  // TODO: Report the error
	  return FALSE;
	}

      strncpy(tex, &ptr1[strlen(startdelim)], pos2 - pos1 - strlen(startdelim)-strlen(enddelim));
      tex[pos2-pos1-strlen(startdelim)-strlen(enddelim)] = '\0';

      // Pidgin transforms & to &amp; and I make the inverse transformation
      while ( (filter = strstr(tex, FILTER_AND) ) != NULL)
	{
	  strcpy(&tex[strlen(tex) - strlen(filter) + 1], &filter[5]);
	}
      // Pidgin transforms < to &lt
      while ( (filter = strstr(tex, FILTER_LT) ) != NULL)
	{
	  strcpy(&tex[strlen(tex) - strlen(filter)], "<");
	  strcpy(&tex[strlen(tex) - strlen(filter)] + 1, &filter[4]);
	}
      // Pidgin transforms > to &gt
      while ( (filter = strstr(tex, FILTER_GT) ) != NULL)
	{
	  strcpy(&tex[strlen(tex) - strlen(filter)], ">");
	  strcpy(&tex[strlen(tex) - strlen(filter)] + 1, &filter[4]);
	}
      // <br> filter
      while ( (filter = strstr(tex, FILTER_BR) ) != NULL)
	{
	  strcpy(&tex[strlen(tex) - strlen(filter)], &filter[4]);
	}
      
      // Creates the image in file_png
      if (!latex_to_image(tex, &file_tex, &file_dvi, &file_ps, &file_png)) { free(tex); return FALSE; };
      free(tex);

      // loading image
       if (!g_file_get_contents(file_png, &filedata, &size, &error))
	{
	  purple_notify_error(NULL, "LaTeX", error->message, NULL);
	  g_error_free(error);
	  return FALSE;
	}

      unlink(file_tex);
      file_tex[strlen(file_tex)-4]='\0';

      strcat(file_tex, ".aux");
      unlink(file_tex);
      file_tex[strlen(file_tex)-4]='\0';
      strcat(file_tex, ".log");
      unlink(file_tex);
      unlink(file_dvi);
      unlink(file_ps);
      unlink(file_png);

      free(file_tex);
      free(file_dvi);
      free(file_ps);
      free(file_png);

      
      name = "pidginTeX.jpg";

      idimg = purple_imgstore_add_with_id(filedata, MAX(1024,size), name);
      filedata = NULL;

      if (idimg == 0)
	{
	  buf = g_strdup_printf("Failed to store image.");
	  purple_notify_error(NULL,"LaTeX", buf, NULL);
	  g_free(buf);
	  return FALSE;
	}

      idstring = malloc(10);
      sprintf(idstring, "%d", idimg);

      // making new message
      if ((message = malloc (strlen(*tmp2) - pos2 + pos1 + strlen(idstring) + strlen(IMG_BEGIN) + strlen(IMG_END) + 1)) == NULL)
	{
	  purple_notify_error(NULL,"LaTeX", "couldn't make the message.", NULL);
	  return FALSE;
	}

      if (pos1 > 0)
	{
	  strncpy(message, *tmp2, pos1);
	  message[pos1] = '\0';
	  strcat(message, IMG_BEGIN);
	} else  {
	  strcpy(message, IMG_BEGIN);
	}
      strcat(message, idstring);
      strcat(message, IMG_END);

      free(idstring);

      if (pos2 < strlen(*tmp2))
      { strcat(message, &ptr2[strlen(enddelim)]); }

      free(*tmp2);
      if ((*tmp2 = malloc(strlen(message)+1)) == NULL)
      {
	purple_notify_error(NULL,"LaTeX", "couldn't split the message.", NULL);
	return FALSE;
      }

      strcpy(*tmp2, message);
      free(message);

      ptr1 = strstr(*tmp2, startdelim);
    }
  return TRUE;
}

static gboolean pidgin_latex_write(PurpleConversation *conv, char *nom, char *message, PurpleMessageFlags messFlag, char *original)
{
  gboolean logflag;

  // writing log
  logflag = purple_conversation_is_logging(conv);

  if (logflag)
    {
      GList *log;

      if (conv->logs == NULL)
        open_log(conv);

      log = conv->logs;
      while (log != NULL) {
        purple_log_write((PurpleLog *)log->data, messFlag, nom, time(NULL), original);
        log = log->next;
      }
      purple_conversation_set_logging(conv,FALSE);
    }
  
  purple_conv_im_write(PURPLE_CONV_IM(conv), nom, message, messFlag, time(NULL));
 
  if (logflag)
    purple_conversation_set_logging(conv,TRUE);

  return FALSE;
}

static gboolean message_send(PurpleAccount *account, const char *who, char **buffer, PurpleConversation *conv, PurpleMessageFlags flags)
{
  char *tmp2;
  int t[10], i = 0;//, j;

  // if nothing to do
  if (/*strstr(*buffer, BEG) == NULL &&*/ strstr(*buffer,KOPETE_TEX) == NULL)
    {
      return FALSE;
    };

  if (is_blacklisted(*buffer)) return FALSE;

  if((tmp2 = malloc(strlen(*buffer)+1)) == NULL)
    {
      // TODO: Notify Error
      return FALSE;
    }

  strcpy(tmp2,*buffer);

  if (analyse(conv, &tmp2, KOPETE_TEX, KOPETE_TEX, FALSE) )
    {
      char *name2;

      // TODO : THIS IS VERY VERY UGLY
      // TODO : MODIFY TO MINIMIZE CALL
      // finding name of sender
      if (purple_account_get_alias(account) != NULL)
	{
	  name2 = malloc(strlen(purple_account_get_alias(account))+1);
	  strcpy(name2, purple_account_get_alias(account));
	}
      else	if (purple_account_get_username(account) != NULL)
	{
	  name2 = malloc(strlen(purple_account_get_username(account))+1);
	  strcpy(name2,purple_account_get_username(account));
	} else {
	  free(tmp2);
	  return FALSE;
	}

      pidgin_latex_write(conv, name2, tmp2, PURPLE_MESSAGE_SEND, *buffer);

      free(tmp2);
      free(name2);

      return TRUE;
    }

  free(tmp2);

  return FALSE;
}

static gboolean message_recv(PurpleAccount *account, char **sender, char **buffer, PurpleConversation *conv, PurpleMessageFlags *flags)
{

  // if no $$ -> nothing to do
  if (strstr(*buffer, BEG) == NULL && strstr(*buffer, KOPETE_TEX) == NULL)
    {
      return FALSE;
    };

  if (is_blacklisted(*buffer)) return FALSE;

  if (conv==NULL) conv = purple_conversation_new(PURPLE_CONV_TYPE_IM,account,*sender);
  if (purple_conversation_get_im_data(conv) != NULL)
    {
      char *tmp2;
      int t[10], i = 0;//, j;

      if ((tmp2 = malloc(strlen(*buffer)+1)) == NULL)
	{
	  // TODO: Report the error
	  return FALSE;
	}

      strcpy(tmp2,*buffer);

      if (analyse(conv, &tmp2, KOPETE_TEX, KOPETE_TEX, TRUE));
      {
      	pidgin_latex_write(conv, *sender, tmp2, PURPLE_MESSAGE_RECV, *buffer);

	free(tmp2); tmp2 = NULL;
	free(*buffer);
	*buffer = NULL;
	return TRUE;
      }

      free(tmp2);
      return FALSE;
    }

  return FALSE;
}


static gboolean plugin_load(PurplePlugin *plugin)
{
  void *conv_handle = purple_conversations_get_handle();

  purple_signal_connect(conv_handle, "writing-im-msg",
		      plugin, PURPLE_CALLBACK(message_send), NULL);

  purple_signal_connect_priority(conv_handle, "receiving-im-msg",
		      plugin, PURPLE_CALLBACK(message_recv), NULL, -9999);
  purple_debug(PURPLE_DEBUG_INFO, "LaTeX", "LaTeX loaded\n");

  return TRUE;
}

static gboolean plugin_unload(PurplePlugin * plugin)
{
  void *conv_handle = purple_conversations_get_handle();

  purple_signal_disconnect(conv_handle, "writing-im-msg", plugin, PURPLE_CALLBACK(message_send));
  purple_signal_disconnect(conv_handle, "receiving-im-msg", plugin, PURPLE_CALLBACK(message_recv));

  return TRUE;
}


static PurplePluginInfo info =
  {
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_STANDARD,                             /**< type           */
    NULL,                                             /**< ui_requirement */
    0,                                                /**< flags          */
    NULL,                                             /**< dependencies   */
    PURPLE_PRIORITY_DEFAULT,                            /**< priority       */

    LATEX_PLUGIN_ID,                                  /**< id             */
    "LaTeX",                                      /**< name           */
    "1.1",                                        /**< version        */
    /**  summary        */
    "To display LaTeX formula into Pidgin conversation.",
    /**  description    */
    "Put LaTeX-code between $$ ... $$ markup to have it displayed as Picture in your conversation.\nRemember that your contact needs an similar plugin or else he will just see the pure LaTeX-code\nYou must have LaTeX and ImageMagick installed (in your PATH)",
    "Benjamin Moll <qjuh@users.sourceforge.net>\nNicolas Schoonbroodt <nicolas@ffsa.be>\nNicolai Stange <nic-stange@t-online.de>",   /**< author       */
    WEBSITE,                                          /**< homepage       */
    plugin_load,                                      /**< load           */
    plugin_unload,                                    /**< unload         */
    NULL,                                             /**< destroy        */
    NULL,                                             /**< ui_info        */
    NULL,                                             /**< extra_info     */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  };



static void
init_plugin(PurplePlugin *plugin)
{
}

PURPLE_INIT_PLUGIN(LaTeX, init_plugin, info)
