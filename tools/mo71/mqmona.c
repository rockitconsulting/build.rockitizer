/**********************************************************************/
/*  FILE   : MQMONA.C                                                 */
/*  PURPOSE: MQSeries Command Server Agent                            */
/*           This program will convert multiple responses from a      */
/*           command server into a single response.                   */
/*                                                                    */
/* <START_COPYRIGHT>                                                  */
/* Licensed Materials - Property of IBM                               */
/*                                                                    */
/* 5724-B41                                                           */
/* (C) Copyright IBM Corp. 1994, 2004 All Rights Reserved.            */
/*                                                                    */
/* US Government Users Restricted Rights - Use, duplication or        */
/* disclosure restricted by GSA ADP Schedule Contract with            */
/* IBM Corp.                                                          */
/* <END_COPYRIGHT>                                                    */
/*                                                                    */
/* AUTHOR : Paul Clarke  paulg_clarke@uk.ibm.com                      */
/*                                                                    */
/* Please note:                                                       */
/* THIS PROGRAM IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,     */
/* EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,          */
/* THE IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A         */
/* PARICULAR PURPOSE.                                                 */
/*                                                                    */
/* In other words:                                                    */
/* "If it don't work properly - don't blame me"                       */
/**********************************************************************/
                                       /* Include files               */
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "cmqc.h"
#include "cmqcfc.h"
#include "time.h"
                                       /* Macros                      */
#ifndef TRUE
  #define TRUE  1
#endif
#ifndef FALSE
  #define FALSE 0
#endif

#define SAME8(a,b) (*(((PULONG)(a))  ) == *(((PULONG)(b))  )  &&  \
                    *(((PULONG)(a))+1) == *(((PULONG)(b))+1) )

#define VB_COUNTS   0x00000001
#define VB_HIGH     0x00000002
#define VB_MEDIUM   0x00000004
#define VB_LOW      0x00000008
                                       /* Typedefs                    */
typedef struct _REQUEST
{
  struct _REQUEST * Next;
  time_t            Time;
  MQBYTE48          ReplyToQ;
  MQBYTE48          ReplyToQMgr;
  MQBYTE24          MsgId;
  MQLONG            ReportOptions;
  MQBYTE24          OrigMsgId;
  MQBYTE24          OrigCorrelId;
  char            * pMsg;
  MQLONG            MsgLen;
  MQLONG            MsgSize;
  MQCHAR8           Format;
} REQUEST;

typedef struct _PARMS
{
  char            * Name;
  char            * Desc;
  int             * Value;
} PARMS;

typedef struct _VERBOSE
{
  char              c;
  char            * Desc;
  int               Value;
} VERBOSE;


typedef unsigned long * PULONG;
                                       /* Static variables            */
MQHCONN   hQm       = MQHC_UNUSABLE_HCONN;
MQHOBJ    hCmdQ     = MQHO_UNUSABLE_HOBJ;
MQHOBJ    hQ        = MQHO_UNUSABLE_HOBJ;
MQHOBJ    hReplyQ   = MQHO_UNUSABLE_HOBJ;
/**********************************************************************/
/* Parameters with default values                                     */
/**********************************************************************/
char      Qm[50]    = "";
char      Q[50]     = "MQMONA";
char      ReplyQ[50]= "SYSTEM.DEFAULT.MODEL.QUEUE";
char      ActReplyQ[50];

#ifdef MVS
char      CmdQ[50]  = "SYSTEM.COMMAND.INPUT";
int       zOS       = TRUE;
#else
char      CmdQ[50]  = "SYSTEM.ADMIN.COMMAND.QUEUE";
int       zOS       = FALSE;
#endif

MQLONG    OutStanding = 0;
int       AgeOut      = 10;      /* Number of seconds before requests */
                                 /* are aged out                      */
int       ReqWait     = 300;
int       ReqCheck    = 0;
int       RepWait     = 1;
int       RepCheck    = 0;
int       MaxMsgSize  = 100000;
int       Compress    = TRUE;
int       CountDelay  = 0;

int       Verbose     = VB_LOW;

PARMS     Parms[]     = { {"maxage"  ,"Maximum request age  :" ,&AgeOut     },
                          {"maxsize" ,"Maximum message size :" ,&MaxMsgSize },
                          {"cntdel"  ,"Minimum count delay  :" ,&CountDelay },
                          {"reqwait" ,"Request Wait Period  :" ,&ReqWait    },
                          {"reqcheck","Request Check Period :" ,&ReqCheck   },
                          {"repwait" ,"Reply Wait Period    :" ,&RepWait    },
                          {"repcheck","Reply Check Period   :" ,&RepCheck   } };

VERBOSE   VB[]        = { {'l'       ,"Low detail"             ,VB_LOW },
                          {'m'       ,"Medium detail"          ,VB_LOW | VB_MEDIUM },
                          {'h'       ,"High detail"            ,VB_LOW | VB_MEDIUM | VB_HIGH },
                          {'c'       ,"Message Counts"         ,VB_COUNTS },
                          {'q'       ,"Quiet"                  ,0         } };

unsigned int     CountReqMsgs  = 0;
unsigned int     CountReqBytes = 0;
unsigned int     CountRepMsgs  = 0;
unsigned int     CountRepBytes = 0;
unsigned int     CountResMsgs  = 0;
unsigned int     CountResBytes = 0;
REQUEST * pRequests;
time_t    Now;
time_t    LastCount;

                                 /* GetOpt Definitions                */
#define EMPTY_MSG  ""
#define BADCH ('?')
#define COLON (':')

static char *place = EMPTY_MSG;        /* option letter processing    */

#ifdef _WIN32
int    optind=1;                 /* index into parent argv vector     */
int    optopt;                   /* character checked for validity    */
char   *optarg;                  /* argument associated with option   */
/**********************************************************************/
/* Function: getopt                                                   */
/* Purpose : Simulate the UNIX getopt function                        */
/*           Each time this function is called it returns the next    */
/*           option letter and sets a static pointer pointing to      */
/*           any option argument.                                     */
/**********************************************************************/
int getopt(int argc, char **argv, char *ostr)
{
   char *oli = NULL;                   /* option letter list index    */
   unsigned int i;                     /* loop index                  */
   int   found = 0;                    /* found flag                  */

   if (!place || !*place)
   {                                   /* update scanning pointer     */
     place = argv[optind];
     /****************************************************************/
     /* Check for end of input                                       */
     /****************************************************************/

     if ((optind >= argc) ||
         ((*place != '-') && (*place != '/')) ||
         (!*++place))
     {
        return EOF;
     }
   }

   /******************************************************************/
   /* option letter okay?                                            */
   /******************************************************************/

   optopt = (int)*place++;             /* Current option             */

   for (i=0; (i < strlen(ostr)) && !found; i++)
   {                            /* check in option string for option */
     if (optopt == ostr[i])
     {
       found = 1;                      /* option is present          */
       oli = &ostr[i];                 /* set oli to point to it     */
     }
   } /* End check in option string for option */

   if (optopt == (int)COLON || !oli)

   {                                   /* Current option not valid   */
      if (!*place)
      {
        ++optind;
      }
      return BADCH;
   } /* End current option not valid */

   /******************************************************************/
   /* Check if this option requires an argument                      */
   /******************************************************************/

   if (*++oli != COLON)
   {                                   /* this option doesn't require*/
                                       /* an arg                     */
     optarg = NULL;
     if (!*place)
     {
        ++optind;
     }
   }
   else
   {                                   /* this option needs an arg   */
     if (*place)
     {
        optarg = place;                /* parm follows arg directly  */
        if (*optarg == COLON)
          optarg++;                    /* Remove colon if present    */
     }
     else
     {
       if (argc <= ++optind)
       {                               /* No argument found          */
         place = EMPTY_MSG;
         return BADCH;
       }
       else
       {
         optarg = argv[optind];        /* white space                */
         if (*optarg == COLON)
           optarg++;                   /* Remove colon if present    */
       }
     }

     place = EMPTY_MSG;
     ++optind;
   }

   return optopt;                      /* return option letter       */
}

#endif
/**********************************************************************/
/* Function : Usage                                                   */
/* Purpose  : Print out MQMONA command format                         */
/**********************************************************************/
static void Usage(void)
{
  int i;
  printf("\nUsage: MQMONA <Optional flags as below>\n");
  printf("        [-c <Command Queue>     ]\n");
  printf("        [-m <Queue Manager Name>]\n");
  printf("        [-n                     ] No compression\n");
  printf("        [-q <Queue Name>        ]\n");
  printf("        [-z                     ] Queue Manager is z/OS\n");
  printf("        [-p {parm=<value> [,] } ]\n");
  printf("          where parms are:\n");
  for (i=0; i<sizeof(Parms)/sizeof(Parms[0]) ;i++)
  {
     printf("             %s  %s\n",Parms[i].Desc,Parms[i].Name);
  }
  printf("        [-v {<value>}           ] Verbosity level\n");
  printf("          where values are:\n");
  for (i=0; i<sizeof(VB)/sizeof(VB[0]) ;i++)
  {
     printf("             %c  %s\n",VB[i].c,VB[i].Desc);
  }
}
/**********************************************************************/
/* Function : OpenQueue                                               */
/* Purpose  : Open a Queue given the open options                     */
/**********************************************************************/
MQHOBJ OpenQueue(char * Q, char * ActQ, MQLONG OpenOptions)
{
  MQHOBJ hObj = MQHO_UNUSABLE_HOBJ;
  MQOD   mqod = { MQOD_DEFAULT };
  MQLONG CompCode,Reason;
  char * pSp;

  strncpy(mqod.ObjectName,Q,sizeof(mqod.ObjectName));

  MQOPEN(hQm,&mqod,OpenOptions,&hObj,&CompCode,&Reason);
  if (CompCode == MQCC_FAILED)
  {
    printf("MQOPEN('%s') failed RC(%d)\n",Q,Reason);
  }
  if (ActQ)
  {
    memcpy(ActQ,mqod.ObjectName,sizeof(mqod.ObjectName));
    ActQ[MQ_Q_NAME_LENGTH] = 0;
    pSp = strchr(ActQ,' ');
    if (pSp) *pSp = 0;
  }
  return hObj;
}

/**********************************************************************/
/* Function : ProcessQueue                                            */
/* Purpose  : Reads each message off a queue and calls the callback   */
/*            function                                                */
/*            Returns TRUE if the program should end                  */
/**********************************************************************/
int ProcessQueue(MQLONG    wait,
                 char    * Q,
                 MQHOBJ    hObj,
                 int (* fn)(MQMD * pMqmd,char * pMsg,MQLONG MsgLen))
{
  MQMD   mqmd     = { MQMD_DEFAULT};
  MQGMO  gmo      = { MQGMO_DEFAULT };
  MQLONG MsgLen;
  MQLONG CompCode,Reason;
  int    Finished = 0;

  static char   *  pMsg        = NULL;
  static MQLONG    MsgSize     = 0;

  if (Verbose & VB_HIGH)
  {
    printf("Processing Queue '%s' Wait(%d)\n",Q,wait);
  }
  if (MsgSize < 4096) MsgSize = 4096;

  gmo.Options = MQGMO_NO_SYNCPOINT      |
                MQGMO_FAIL_IF_QUIESCING |
                MQGMO_CONVERT           |
                MQGMO_WAIT;
  gmo.WaitInterval = wait * 1000;

  while(1)
  {
    if (!pMsg)
    {
      pMsg = malloc(MsgSize);
      if (!pMsg)
      {
        printf("Can not allocate %d bytes for a message ! \n",MsgSize);
        Finished = 1;
        goto MOD_EXIT;
      }
    }
    memset(mqmd.MsgId   ,0,sizeof(mqmd.MsgId));
    memset(mqmd.CorrelId,0,sizeof(mqmd.CorrelId));

    MQGET(hQm,
          hObj,
         &mqmd,
         &gmo,
          MsgSize,
          pMsg,
         &MsgLen,
         &CompCode,
         &Reason );
    switch(Reason)
    {
      case MQRC_TRUNCATED_MSG_FAILED:
           free(pMsg);
           pMsg = NULL;
           MsgSize = (MsgLen + 0x01000) & 0xFFFFF000;
           break;
      case 0:
           Finished = fn(&mqmd,pMsg,MsgLen);
           if (Finished) goto MOD_EXIT;
                                       /* Just wait a little bit      */
           gmo.WaitInterval = 100;
           break;
      case MQRC_NO_MSG_AVAILABLE:
           goto MOD_EXIT;
           break;
      default:
           printf("MQGET('%s') failed RC(%d)\n",Q,Reason);
           Finished = 1;
           goto MOD_EXIT;
           break;
    }
  }
MOD_EXIT:
  return Finished;
}
/**********************************************************************/
/* Function : CommandMessage                                          */
/* Purpose  : Process a message read off our command queue            */
/*            Returns TRUE if the program should end                  */
/**********************************************************************/
int CommandMessage(MQMD * mqmd, char * pMsg, MQLONG MsgLen)
{
  int       Finished = FALSE;
  MQPMO     pmo      = { MQPMO_DEFAULT };
  REQUEST * pRequest = NULL;
  MQLONG    CompCode,Reason;

  if (Verbose & VB_HIGH)
  {
    printf("CMD:Received message of length %d bytes\n",MsgLen);
  }
  if (mqmd -> ReplyToQ[0] != ' ' && mqmd -> ReplyToQMgr[0] != ' ')
  {
    pRequest = malloc(sizeof(REQUEST));
    if (!pRequest)
    {
      printf("Can not allocate a REQUEST structure\n");
      Finished = 1;
      goto MOD_EXIT;
    }
    time(&Now);
    pRequest -> Time = Now;
    pRequest -> pMsg = NULL;
    memcpy(pRequest->ReplyToQ    , mqmd -> ReplyToQ   , MQ_Q_NAME_LENGTH);
    memcpy(pRequest->ReplyToQMgr , mqmd -> ReplyToQMgr, MQ_Q_MGR_NAME_LENGTH);

    strncpy(mqmd -> ReplyToQ   , ActReplyQ, MQ_Q_NAME_LENGTH);
    mqmd->ReplyToQMgr[0] = 0;

    memcpy(pRequest->OrigMsgId    , mqmd -> MsgId      , MQ_MSG_ID_LENGTH);
    memcpy(pRequest->OrigCorrelId , mqmd -> CorrelId   , MQ_CORREL_ID_LENGTH);
    pRequest->ReportOptions = mqmd -> Report;

    mqmd -> Report = MQRO_PASS_MSG_ID |
                    (pRequest->ReportOptions | MQRO_DISCARD_MSG);

    memset(mqmd -> MsgId   , 0, MQ_MSG_ID_LENGTH);
  }
                                       /* Now, put it to the server   */
  pmo.Options = MQPMO_NO_SYNCPOINT |
                MQPMO_PASS_ALL_CONTEXT;

  pmo.Context = hQ;

  MQPUT(hQm,
        hCmdQ,
        mqmd,
       &pmo,
        MsgLen,
        pMsg,
       &CompCode,
       &Reason);
  if (CompCode == MQCC_FAILED)
  {
    printf("MQPUT('%s') failed RC(%d)\n",CmdQ,Reason);
    Finished = 1;
    goto MOD_EXIT;
  }
                                       /* Update stats                */
  CountReqMsgs ++;
  CountReqBytes += MsgLen;

  if (pRequest)
  {
                                       /* Save the Message Id         */
    memcpy(pRequest -> MsgId, mqmd -> MsgId, MQ_MSG_ID_LENGTH);
    OutStanding ++;
    pRequest -> Next = pRequests;
    pRequests        = pRequest;
  }
MOD_EXIT:
  return Finished;
}
/**********************************************************************/
/* Function : CheckMessage                                            */
/* Purpose  : Check what to do with this message                      */
/**********************************************************************/
void CheckMessage(REQUEST * pRequest,
                  MQMD    * mqmd,
                  char    * pMsg,
                  MQLONG    MsgLen,
                  int     * Save,
                  int     * Join,
                  int     * Finalise)
{
                                       /* Set defaults                */
  *Save     = FALSE;
  *Join     = FALSE;
  *Finalise = FALSE;

  /********************************************************************/
  /* Is this a PCF message                                            */
  /********************************************************************/
  if (SAME8(mqmd->Format,MQFMT_ADMIN))
  {
    MQCFH * pCfh = (MQCFH *)pMsg;
                                      /* Is this the last response    */
    if (pCfh -> Control & MQCFC_LAST)
    {
      if (pRequest -> pMsg) *Join = TRUE;
      *Finalise = TRUE;
    }
    else
    {
                                      /* Always join with any previous*/
      *Join     = TRUE;
    }
  }
  else
  if (SAME8(mqmd->Format,MQFMT_COMMAND_1) ||
      SAME8(mqmd->Format,MQFMT_COMMAND_2))
  {
    /******************************************************************/
    /* Have we a previous message ?                                   */
    /******************************************************************/
    if (pRequest->pMsg)
    {
      *Join = TRUE;
    }
    else
    {
      *Save = TRUE;
    }
    if (SAME8(pMsg,"CSQ9022I") ||
        SAME8(pMsg,"CSQ9023E") ||
        SAME8(pMsg,"CSQ9023I"))        /* CSQ9023I doesn't exist !    */
    {
      *Finalise = TRUE;
    }
  }
  else
  {
    *Finalise = TRUE;
  }
}
/**********************************************************************/
/* Function : SendMessage                                             */
/* Purpose  : Send a given message to the reply queue                 */
/**********************************************************************/
int SendMessage(REQUEST * pRequest,
                MQMD    * mqmd,
                char    * pMsg,
                MQLONG    MsgLen)
{
  int       Finished  = FALSE;
  MQOD      mqod      = { MQOD_DEFAULT };
  MQPMO     pmo       = { MQPMO_DEFAULT };
  MQLONG    CompCode,Reason;
                                       /* Forward this message        */
  memcpy(mqod.ObjectName    , pRequest -> ReplyToQ   , MQ_Q_NAME_LENGTH);
  memcpy(mqod.ObjectQMgrName, pRequest -> ReplyToQMgr, MQ_Q_MGR_NAME_LENGTH);

  /********************************************************************/
  /* Were we asked to maintain the MsgId or get a new one ?           */
  /********************************************************************/
  if (pRequest -> ReportOptions & MQRO_PASS_MSG_ID)
     memcpy(mqmd->MsgId,pRequest->OrigMsgId,MQ_MSG_ID_LENGTH);
  else
     memset(mqmd->MsgId,0,MQ_MSG_ID_LENGTH);

  /********************************************************************/
  /* Now, either we set the CorrelId to the original CorrelId or      */
  /* to the Original MsgId                                            */
  /********************************************************************/
  if (pRequest -> ReportOptions & MQRO_PASS_CORREL_ID)
     memcpy(mqmd->CorrelId,pRequest->OrigCorrelId,MQ_MSG_ID_LENGTH);
  else
     memcpy(mqmd->CorrelId,pRequest->OrigMsgId,MQ_MSG_ID_LENGTH);

  /********************************************************************/
  /* Ok, go ahead and put it to the reply                             */
  /********************************************************************/
  pmo.Options = MQPMO_NO_SYNCPOINT |
                MQPMO_PASS_ALL_CONTEXT;

  pmo.Context = hReplyQ;

  MQPUT1(hQm,
        &mqod,
         mqmd,
        &pmo,
         MsgLen,
         pMsg,
        &CompCode,
        &Reason);
  if (CompCode == MQCC_FAILED)
  {
    printf("MQPUT1('%s/%s') failed RC(%d)\n",
            mqod.ObjectQMgrName,mqod.ObjectName,Reason);
    goto MOD_EXIT;
  }
                                       /* Update stats                */
  CountResMsgs ++;
  CountResBytes += MsgLen;

MOD_EXIT:
  return Finished;
}
/**********************************************************************/
/* Function : SaveMessage                                             */
/* Purpose  : Save this message                                       */
/**********************************************************************/
int SaveMessage(REQUEST * pRequest,
                MQMD    * mqmd,
                char    * pMsg,
                MQLONG    MsgLen)
{
  int Finished = FALSE;
  /********************************************************************/
  /* Send a message if we have one                                    */
  /********************************************************************/
  if (pRequest->pMsg)
  {
    if (pRequest->MsgLen)
    {
      SendMessage(pRequest,mqmd,pRequest->pMsg,pRequest->MsgLen);
    }
                                       /* Is it big enough ?          */
    if (pRequest->MsgSize < MsgLen)
    {
      free(pRequest->pMsg);
      pRequest->pMsg=NULL;
    }
  }

  if (!pRequest->pMsg)
  {
    pRequest->MsgSize = MsgLen;
    pRequest->pMsg    = malloc(pRequest->MsgSize);
    if (!pRequest->pMsg)
    {
      printf("Can not allocate %d bytes for a message ! \n",pRequest->MsgSize);
      Finished = 1;
      goto MOD_EXIT;
    }
  }
  memcpy(pRequest->pMsg,pMsg,MsgLen);
  pRequest->MsgLen = MsgLen;

MOD_EXIT:
  return Finished;
}
/**********************************************************************/
/* Function : JoinMQSCMessage                                         */
/* Purpose  : Join two MQSCs together                                 */
/**********************************************************************/
int JoinMQSCMessage(REQUEST * pRequest,
                    MQMD    * mqmd,
                    char    * pMsg,
                    MQLONG    MsgLen)
{
  int      Finished = FALSE;
  MQLONG   Needed   = MsgLen + pRequest->MsgLen + 10;
  char   * p;

  if (Needed > pRequest->MsgSize)
  {
    char * pNewMsg;
                                       /* Round to the nearest 64K    */
    Needed = (Needed + 0x010000) & 0xFFFF0000;

    pNewMsg = malloc(Needed);
    if (!pNewMsg)
    {
      printf("Can not allocate %d bytes for a message ! \n",Needed);
      Finished = 1;
      goto MOD_EXIT;
    }
    memcpy(pNewMsg,pRequest->pMsg,pRequest->MsgLen);
    free(pRequest->pMsg);
    pRequest->pMsg    = pNewMsg;
    pRequest->MsgSize = Needed;
  }

  /********************************************************************/
  /* A 'joined' MQSC response is indicated to the client by having a  */
  /* '*' in the 9th position                                          */
  /********************************************************************/
  p    = pRequest->pMsg;
  p[8] = '*';

  /********************************************************************/
  /* Now, we actually join two MQSC messages putting a NULL character */
  /* between them. We know that a NULL character can't appear in the  */
  /* actual message itself.                                           */
  /********************************************************************/
  p   += pRequest->MsgLen;
  *p++ = 0;
  memcpy(p,pMsg,MsgLen);
  pRequest->MsgLen += MsgLen + 1;

MOD_EXIT:
  return Finished;
}
/**********************************************************************/
/* Function : JoinPCFMessage                                          */
/* Purpose  : Join two PCFs together                                  */
/**********************************************************************/
int JoinPCFMessage(REQUEST * pRequest,
                   MQMD    * mqmd,
                   char    * pMsg,
                   MQLONG    MsgLen)
{
  int      Finished = FALSE;
  MQCFH  * pCfh1    = (MQCFH *)pRequest -> pMsg;
  MQCFH  * pCfh2    = (MQCFH *)pMsg;
  MQLONG   MsgLen1  = pRequest -> MsgLen;
  MQLONG   MsgLen2  = MsgLen;
  MQLONG   Needed;
  MQCFIN * pCfin;
  /********************************************************************/
  /* We only join commands that work !                                */
  /********************************************************************/
  if (pCfh1 -> CompCode || pCfh2 -> CompCode)
  {
    Finished = SaveMessage(pRequest,mqmd,pMsg,MsgLen);
    goto MOD_EXIT;
  }
  /********************************************************************/
  /* How much space do we need                                        */
  /********************************************************************/
  Needed = MsgLen1 + MsgLen2 + sizeof(MQCFIN) - sizeof(MQCFH);
  if (Needed > pRequest->MsgSize)
  {
    char * pNewMsg;
                                       /* Round to the nearest 64K    */
    Needed = (Needed + 0x010000) & 0xFFFF0000;

    pNewMsg = malloc(Needed);
    if (!pNewMsg)
    {
      printf("Can not allocate %d bytes for a message ! \n",Needed);
      Finished = 1;
      goto MOD_EXIT;
    }
    memcpy(pNewMsg,pRequest->pMsg,pRequest->MsgLen);
    free(pRequest->pMsg);
    pRequest->pMsg    = pNewMsg;
    pRequest->MsgSize = Needed;
    pCfh1   = (MQCFH *)pRequest -> pMsg;
  }
  /********************************************************************/
  /* Add a separator                                                  */
  /* Note that what the value is doesn't matter too much but it       */
  /* must be recognised by the client sending in commands (ie MQMON)  */
  /* and it must not be a value currently in use by any MQ command    */
  /*******************************************************************/
  pCfin   = (MQCFIN *)(pRequest->pMsg + pRequest->MsgLen);
  pCfin -> Type        = MQCFT_INTEGER;
  pCfin -> StrucLength = MQCFIN_STRUC_LENGTH;
  pCfin -> Parameter   = 301162;
  pCfin -> Value       = 0;
  /********************************************************************/
  /* Copy the rest of the command                                     */
  /********************************************************************/
  memcpy((pCfin+1),(pCfh2+1),MsgLen2 - sizeof(MQCFH));
  pRequest -> MsgLen += MsgLen2 + sizeof(MQCFIN) - sizeof(MQCFH);

  /********************************************************************/
  /* Now, update the command header                                   */
  /********************************************************************/
  pCfh1 -> ParameterCount += pCfh2 -> ParameterCount + 1;
  pCfh1 -> Control         = pCfh2 -> Control;
MOD_EXIT:
  return Finished;
}
/**********************************************************************/
/* Function : JoinMessage                                             */
/* Purpose  : Join this message with the previous one                 */
/**********************************************************************/
int JoinMessage(REQUEST * pRequest,
                MQMD    * mqmd,
                char    * pMsg,
                MQLONG    MsgLen)
{
  int Finished = FALSE;
                                       /* No previous message ?       */
  if (!pRequest->pMsg)
  {
    Finished = SaveMessage(pRequest,mqmd,pMsg,MsgLen);
    goto MOD_EXIT;
  }
                                       /* No previous message ?       */
  if (!pRequest->MsgLen)
  {
    Finished = SaveMessage(pRequest,mqmd,pMsg,MsgLen);
    goto MOD_EXIT;
  }
  /********************************************************************/
  /* Make sure this is not going to be too big                        */
  /********************************************************************/
  if ((pRequest->MsgLen + MsgLen) > MaxMsgSize)
  {
    Finished = SaveMessage(pRequest,mqmd,pMsg,MsgLen);
    goto MOD_EXIT;
  }
  /********************************************************************/
  /* Is this a PCF message ?                                          */
  /********************************************************************/
  if (SAME8(mqmd->Format,MQFMT_ADMIN))
  {
     Finished = JoinPCFMessage(pRequest,mqmd,pMsg,MsgLen);
  }
  else
  if (SAME8(mqmd->Format,MQFMT_COMMAND_1) ||
      SAME8(mqmd->Format,MQFMT_COMMAND_2))
  {
     Finished = JoinMQSCMessage(pRequest,mqmd,pMsg,MsgLen);
  }

MOD_EXIT:
  return Finished;
}
/**********************************************************************/
/* Function : CompressMQSCMessage                                     */
/* Purpose  : Compress an MQSC message                                */
/*            This esentially just blanks before a ')' character      */
/**********************************************************************/
void CompressMQSCMessage(char * pStartMsg, MQLONG * pMsgLen)
{
  MQLONG  MsgLen = *pMsgLen;
  char  * pEnd   = pStartMsg + MsgLen;
  char  * pIn    = pStartMsg;
  char  * pOut   = pStartMsg;
  char  * pStart;

  while (pIn < pEnd)
  {
    if (*pIn == ' ')
    {
      pStart = pIn;
      pIn++;
      while(*pIn == ' ' && pIn != pEnd) pIn++;
      if (pIn == pEnd || *pIn != ')')
      {
        memset(pOut,' ',pIn - pStart);
        pOut += (pIn - pStart);
      }
    }
    else
    {
      *pOut++ = *pIn++;
    }
  }
  *pMsgLen = pOut - pStartMsg;
}
/**********************************************************************/
/* Function : CompressPCFMessage                                      */
/* Purpose  : Go through a PCF message and compress it                */
/*            This esentially just removes blanks from strings        */
/**********************************************************************/
void CompressPCFMessage(char * pStartMsg, MQLONG * pMsgLen)
{
  MQCFH  * pCfh = (MQCFH *)pStartMsg;
  MQCFST * pCfst;
  MQCFIN * pCfin;
  MQCFBS * pCfbs;
  MQCFSL * pCfsl;
  MQCFIL * pCfil;
  char   * pIn  = (char *)(pCfh + 1);
  char   * pOut = pIn;
  MQLONG   Type;
  MQLONG   Copy;
  MQLONG   Move;
  MQLONG   Left = *pMsgLen - sizeof(MQCFH);
  MQLONG   ParameterCount = pCfh -> ParameterCount;
  MQLONG   len;
  char   * p;
                                       /* Go through each field       */
  while(ParameterCount && Left > 0)
  {
    Type = *(MQLONG *) pIn;
    ParameterCount --;
    switch(Type)
    {
      case MQCFT_STRING:
           pCfst   = (MQCFST *) pIn;
           Move    = pCfst -> StrucLength;
           /***********************************************************/
           /* Does it need compressing                                */
           /***********************************************************/
           if (pCfst -> StringLength > 4)
           {
             len = pCfst -> StringLength;
             p   = pCfst -> String + len - 1;
             while (len && *p == ' ') { p--; len--;};
             pCfst -> StringLength = len;
             pCfst -> StrucLength  = (MQCFST_STRUC_LENGTH_FIXED + len + 3) &
                                     0xFFFFFFFC;
           }
           Copy    = pCfst -> StrucLength;
           break;

      case MQCFT_BYTE_STRING:
           pCfbs  = (MQCFBS *) pIn;
           Copy   = pCfbs -> StrucLength;
           Move   = Copy;
           break;

      case MQCFT_STRING_LIST:
           pCfsl  = (MQCFSL *) pIn;
           Copy   = pCfsl -> StrucLength;
           Move   = Copy;
           break;

      case MQCFT_INTEGER:
           pCfin  = (MQCFIN *) pIn;
           Copy   = pCfin -> StrucLength;
           Move   = Copy;
           break;

      case MQCFT_INTEGER_LIST:
           pCfil  = (MQCFIL *) pIn;
           Copy   = pCfil -> StrucLength;
           Move   = Copy;
           break;

      default:
           printf("Can not compress PCF type %d\n",Type);
           Copy   = Left;
           Move   = Copy;
    }
    Left -= Move;
    if (pIn == pOut)
    {
      pIn  += Move;
      pOut += Copy;
    }
    else
    {
      int Diff = (Move-Copy);
      while (Copy--) *pOut++ = *pIn++;
      pIn += Diff;
    }
  }
  *pMsgLen = (pOut - pStartMsg);
}
/**********************************************************************/
/* Function : ReplyMessage                                            */
/* Purpose  : Process a message read off our reply queue              */
/*            Returns TRUE if the program should end                  */
/**********************************************************************/
int ReplyMessage(MQMD * mqmd, char * pMsg, MQLONG MsgLen)
{
  int       Finished  = FALSE;
  REQUEST * pRequest  = pRequests;
  REQUEST * plRequest = NULL;
  int       Save,Join,Finalise;

  if (Verbose & VB_HIGH)
  {
    printf("REPLY:Received message of length %d bytes\n",MsgLen);
  }
  /********************************************************************/
  /* Find the request in the request list                             */
  /********************************************************************/
  while (pRequest)
  {
    if (!memcmp(pRequest->MsgId,mqmd->MsgId,MQ_MSG_ID_LENGTH)) break;
                                       /* Is this too old             */
    plRequest = pRequest;
    pRequest  = pRequest -> Next;
  }
  if (!pRequest)
  {
    printf("Unrecognised reply received\n");
    goto MOD_EXIT;
  }
                                       /* Update stats                */
  CountRepMsgs ++;
  CountRepBytes += MsgLen;
  /********************************************************************/
  /* Should we compress it ?                                          */
  /********************************************************************/
  if (Compress)
  {
    if (SAME8(mqmd->Format,MQFMT_ADMIN))
    {
      CompressPCFMessage(pMsg,&MsgLen);
    }
    if (SAME8(mqmd->Format,MQFMT_COMMAND_1) ||
        SAME8(mqmd->Format,MQFMT_COMMAND_2))
    {
       CompressMQSCMessage(pMsg,&MsgLen);
    }
  }

  /********************************************************************/
  /* There are a number of actions we might want to do now            */
  /*   SAVE      Save this message (saved message is flushed)         */
  /*   JOIN      Join this with saved message                         */
  /*   FINALISE  This is the last message of this request             */
  /********************************************************************/
  CheckMessage(pRequest,mqmd,pMsg,MsgLen,&Save,&Join,&Finalise);

  if (Join)
  {
    JoinMessage(pRequest,mqmd,pMsg,MsgLen);
  }
  else if (Save)
  {
    SaveMessage(pRequest,mqmd,pMsg,MsgLen);
  }

  if (Finalise)
  {
    if (Join || Save)
      Finished = SendMessage(pRequest,mqmd,pRequest->pMsg,pRequest->MsgLen);
    else
      Finished = SendMessage(pRequest,mqmd,pMsg,MsgLen);

                                       /* Delete this entry           */
    if (plRequest) plRequest -> Next = pRequest -> Next;
              else pRequests         = pRequest -> Next;
    if (pRequest->pMsg) free(pRequest->pMsg);
    free(pRequest);

    OutStanding --;
  }

MOD_EXIT:
  return Finished;
}
/**********************************************************************/
/* Function : FindParm                                                */
/* Purpose  : Returns a point to the parameter                        */
/**********************************************************************/
PARMS * FindParm(char * p,char ** v)
{
  char * e = strchr(p,'=');
  int    i;

  if (!e) return NULL;
  *v = e+1;

  for (i=0; i<sizeof(Parms)/sizeof(Parms[0]);i++)
  {
    if (!memcmp(Parms[i].Name,p,e-p)) return &Parms[i];
  }
  return NULL;
}
/**********************************************************************/
/* Function : main                                                    */
/* Purpose  : The program main entry point                            */
/**********************************************************************/
int main(int     argc, char ** argv)
{
  /* Working variables                                                */
  int     c;
  MQLONG  CompCode,Reason;
  int     Finished = 0;
  MQLONG  delay;
  char  * p,* v,*pTime,*pThen;
  PARMS * pParm;

  /********************************************************************/
  /* V1.0 Original function                                           */
  /* V1.1 MQSC commands joined by NULL character rather than *.       */
  /********************************************************************/
  printf("MQMONA Command Program by Paul Clarke [V1.1] Build:%s\n",
                 __DATE__);
  /********************************************************************/
  /* Grab the parameters from the command line                        */
  /********************************************************************/
  while ((c=getopt(argc, argv,"c:m:np:q:r:v:z?")) !=  EOF)
  {
    switch(c)
    {
                                       /* Command Queue               */
      case 'c':
           strncpy(CmdQ,optarg,sizeof(Q));
           break;
                                       /* Queue Manager Name          */
      case 'm':
           strncpy(Qm,optarg,sizeof(Qm));
           break;
                                       /* Switch off compression      */
      case 'n':
           Compress = FALSE;
           break;
                                       /* Parameters                  */
      case 'p':
           p = optarg;
           while (*p)
           {
             while (*p==' ')p++;
             if (!*p) break;

             pParm = FindParm(p,&v);
             if (!pParm)
             {
               printf("Sorry, don't understand parameter string '%s'\n",optarg);
               Usage();
               goto MOD_EXIT;
             }
             *pParm->Value = atoi(v);
             p = v;
             while (*p>='0' && *p<='9') p++;
             if (*p==',') p++;
           }
           break;
                                       /* Queue Name                  */
      case 'q':
           strncpy(Q,optarg,sizeof(Q));
           break;
                                       /* Reply Queue Name            */
      case 'r':
           strncpy(ReplyQ,optarg,sizeof(ReplyQ));
           break;
                                       /* Vebrose                     */
      case 'v':
           Verbose = 0;
           p = optarg;
           while (*p)
           {
             for (c=0; c<sizeof(VB)/sizeof(VB[0]); c++)
             {
               if (VB[c].c == *p)
               {
                 Verbose |= VB[c].Value;
                 break;
               }
             }
             if (c == sizeof(VB)/sizeof(VB[0]))
             {
               printf("Sorry, don't understand verbose string '%s'\n",optarg);
               Usage();
               goto MOD_EXIT;
             }
             p++;
           }
           break;
                                       /* z/OS                        */
      case 'z':
           zOS = TRUE;
           strcpy(CmdQ,"SYSTEM.COMMAND.INPUT");
           break;
                                       /* Help                        */
      case '?':
      case 'h':
           Usage();
           goto MOD_EXIT;
           break;
                                       /* Unrecognised                */
      default:
           printf("Unrecognised parameter\n");
           Usage();
           goto MOD_EXIT;
           break;
    }
  }
                                       /* No unused arguments ?       */
  if (optind != argc)
  {
    printf("Unrecognised parameter\n");
    Usage();
    goto MOD_EXIT;
  }
  /********************************************************************/
  /* Connect to the Queue Manager                                    */
  /********************************************************************/
  MQCONN(Qm,&hQm,&CompCode,&Reason);
  if (CompCode == MQCC_FAILED)
  {
    printf("MQCONN('%s') failed RC(%d)\n",Qm,Reason);
    goto MOD_EXIT;
  }
  /********************************************************************/
  /* Open the command Queue                                           */
  /********************************************************************/
  hCmdQ = OpenQueue(CmdQ,
                    NULL,
                    MQOO_OUTPUT           |
                    MQOO_PASS_ALL_CONTEXT);
  if (hCmdQ == MQHO_UNUSABLE_HOBJ) goto MOD_EXIT;

  /********************************************************************/
  /* Open our input queue                                             */
  /********************************************************************/
  hQ    = OpenQueue(Q,
                    NULL,
                    MQOO_INPUT_SHARED    |
                    MQOO_SAVE_ALL_CONTEXT);
  if (hQ == MQHO_UNUSABLE_HOBJ) goto MOD_EXIT;

  /********************************************************************/
  /* Open our reply queue                                             */
  /********************************************************************/
  hReplyQ = OpenQueue(ReplyQ,
                      ActReplyQ,
                      MQOO_INPUT_SHARED |
                      MQOO_SAVE_ALL_CONTEXT);
  if (hReplyQ == MQHO_UNUSABLE_HOBJ) goto MOD_EXIT;

  /********************************************************************/
  /* Print out parameters                                            */
  /********************************************************************/
  if (Verbose & VB_LOW)
  {
    printf("Options in effect:\n");

    printf("  Queue Manager        : %s\n",Qm);
    printf("  Input Queue          : %s\n",Q);
    printf("  Command Queue        : %s\n",CmdQ);
    printf("  Reply Queue          : %s\n",ReplyQ);
    if (strcmp(ReplyQ,ActReplyQ))
    {
      printf("  Actual Reply Queue   : %s\n",ActReplyQ);
    }
    for (c=0; c<sizeof(Parms)/sizeof(Parms[0]);c++)
    {
      printf("  %s %d\n",Parms[c].Desc,*Parms[c].Value);
    }
    printf("\n");
  }
  /********************************************************************/
  /* We need to read the input queue and read the reply queue and     */
  /* messages could arrive on either and any time. The right thing to */
  /* do here is have two threads but that would make porting it much  */
  /* more complicated. Since we only need to read the reply queue     */
  /* when we have outstanding requests then we can do a limited poll  */
  /* and keep things a lot simpler.                                   */
  /********************************************************************/
  while (!Finished)
  {
    if (Verbose & VB_COUNTS)
    {
      time(&Now);
      if ((Now - LastCount) >= CountDelay)
      {
        pTime = ctime(&Now);
        printf("%.8s Cmd[%4d,%8d] Rep[%4d,%8d] Res[%4d,%8d] Wait[%4d]\n",
               pTime+11,
               CountReqMsgs,CountReqBytes,
               CountRepMsgs,CountRepBytes,
               CountResMsgs,CountResBytes,
               OutStanding);
        LastCount = Now;
      }
    }
    /******************************************************************/
    /* Have we been sent any new commands to forward                  */
    /******************************************************************/
    delay = OutStanding ? ReqCheck: ReqWait;
    if (ProcessQueue(delay, Q, hQ, CommandMessage)) Finished = 1;

    /******************************************************************/
    /* Have we received any replies                                   */
    /******************************************************************/
    delay = OutStanding ? RepWait : RepCheck;
    if (ProcessQueue(delay, ActReplyQ, hReplyQ, ReplyMessage)) Finished = 1;

    /******************************************************************/
    /* Let's check whether any should be aged out                     */
    /******************************************************************/
    if (OutStanding)
    {
      REQUEST * pRequest  = pRequests;
      REQUEST * plRequest = NULL;
      REQUEST * pnRequest;

      time(&Now);
      while(pRequest)
      {
        if ((Now - pRequest -> Time) > AgeOut)
        {
          char Then[30];
          pThen = ctime(&pRequest->Time);
          strcpy(Then,pThen);
          pTime = ctime(&Now);
          printf("%.8s Ageing out request issued at %.8s\n",pTime+11,Then+11);
          if (plRequest) plRequest -> Next = pRequest -> Next;
                    else pRequests         = pRequest -> Next;
          pnRequest = pRequest -> Next;
          free(pRequest);
          pRequest  = pnRequest;
          OutStanding--;
        }
        else
        {
          plRequest = pRequest;
          pRequest  = pRequest -> Next;
        }
      }
    }
  }

MOD_EXIT:
  if (hQm != MQHC_UNUSABLE_HCONN) MQDISC(&hQm,&CompCode,&Reason);

#ifdef _DEBUG
  {
    char b[100];
    printf("Program Terminated - Press Enter\n");
    gets(b);
  }
#endif
}
