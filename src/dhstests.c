
/*
 *	From A.Bridger@roe.ac.uk Tue Jun 30 15:08:36 1998
 *	Date: Tue, 30 Jun 1998 09:37:18 +0100 (BST)
 *	From: Alan Bridger <A.Bridger@roe.ac.uk>
 *	To: Steven Beard <S.Beard@roe.ac.uk>
 *	Cc: Alan Bridger <A.Bridger@roe.ac.uk>
 *	Subject: dhs tests
 *	
 *	Steven -
 *	
 *	Here is the dhs test code, followed by the script I use to load the
 *	libraries and start imp master
 *	
 *	Cheers
 *	Alan
 */

#include "vxWorks.h"
#include "taskLib.h"
#include "sysLib.h"
#include "stdio.h"

#ifndef vxWorks
#define vxWorks
#endif
#include "dhs.h"

DHS_CONNECT     dhsConnection;
DHS_STATUS      mydhsStatus;

void          export()
{

  int             i, j;
  float           localData[3][2];
  float          *pDhsData;
  char           *dataLabel;
  char            label[50];
  char           *axisLabel[2]={ "None", "None"};
  DHS_BD_DATASET  dataset;
  DHS_BD_FRAME    dataFrame;
  unsigned long   dims[1];
  unsigned long   axisSize[2];
  unsigned long   origin[2];
/*  float           intData[3][2]; */
  DHS_TAG         putTag;
  DHS_CMD_STATUS  sendStatus;
  char            msg[80];
  unsigned long   axisDims[1];

  for (i=0; i<3; i++) {
    for (j=0;j<2;j++) {
      localData[i][j] = 1.0;
    }
  }

  /* setup for new frame */
  /* get data label */
  dataLabel = dhsBdName (dhsConnection, &mydhsStatus);
  sprintf (label, "%s.0.0", dataLabel);
  printf ("label = %s\n", label);

  /* create new dataset */
  dims[0] = 2;
  axisDims[0] = 1;
  axisSize[0] = 3;  
  axisSize[1] = 2;
  origin[0] = 1;
  origin[1] = 1;
  printf ("%s %s\n", axisLabel[0], axisLabel[1]);

  dataset = dhsBdDsNew (&mydhsStatus);

  /* Mandatory dataset headers */
  dhsBdAttribAdd (dataset, "instrument", DHS_DT_STRING, 0, NULL, "agwps0", \
		  &mydhsStatus);
  dhsBdAttribAdd (dataset, "telescope", DHS_DT_STRING, 0, NULL, "UKATC", \
		  &mydhsStatus);
  printf ("After attribadd, dhsStatus=%d\n",mydhsStatus);

  /* Create frame */
  dataFrame = dhsBdFrameNew (dataset, "dataArray", 0, DHS_DT_FLOAT, 2, axisSize, \
			     (const void **) &pDhsData, &mydhsStatus);
  printf ("After framenew, dhsStatus=%d\n",mydhsStatus);

  /* add frame header info. */
  dhsBdAttribAdd (dataFrame, "dataType", DHS_DT_STRING, 0, NULL, "Electrons", \
		  &mydhsStatus);
  dhsBdAttribAdd (dataFrame, "origin", DHS_DT_INT32, 1, dims, origin, \
		  &mydhsStatus);
  dhsBdAttribAdd (dataFrame, "frameName", DHS_DT_STRING, 0, NULL, "A&G Test", \
		  &mydhsStatus);
  dhsBdAttribAdd (dataFrame, "frameId", DHS_DT_STRING, 0, NULL, "0", \
		  &mydhsStatus);
  dhsBdAttribAdd (dataFrame, "axisSize", DHS_DT_INT32, 1, dims, axisSize, \
		  &mydhsStatus);
  dhsBdAttribAdd (dataFrame, "axisLabel", DHS_DT_STRING, 1, dims, axisLabel, \
		  &mydhsStatus);
  printf ("After attribadds, dhsStatus=%d\n",mydhsStatus);

  /* and the data frame */
  printf ("Create the data...\n");

  for (i=0; i<3*2;i++) {
    *pDhsData++ = (float) i;
  }

  dhsBdDsPrint (dataset, &mydhsStatus);

  printf ("Sending it...\n");

  /* send the data to the dhs */
  putTag = dhsBdPut (dhsConnection, label, DHS_BD_PT_DS, DHS_TRUE, dataset, \
		     NULL, &mydhsStatus);
  printf ("After dhsBdPut, dhsStatus=%d\n",mydhsStatus);

  /* wait for completion */
  dhsWait (1, &putTag, &mydhsStatus);
  printf ("After dhsWait, dhsStatus=%d\n",mydhsStatus);
  sendStatus = dhsStatus (putTag, &msg, &mydhsStatus);
  if (sendStatus != DHS_CS_DONE) {
    printf ("Failed to export the data!\n, %s\n", msg);
    printf ("sendStatus=%d\n",sendStatus);
    printf ("%s\n", msg);
  }
  dhsTagFree (putTag, &mydhsStatus);

  if (mydhsStatus != DHS_S_SUCCESS) {
    printf ("Bad dhs status %d \n", mydhsStatus);
    mydhsStatus = DHS_S_SUCCESS;
  }else{
    printf ("Frame exported\n");
  }
}


void          quicklook(
  int             nloop
)
{

  int             i, j, k, l;
  unsigned short  *pDhsData;
  unsigned short  *ptr;
  char           *dataLabel;
  char            label[50];
  char           *axisLabel[2]={ "None", "None"};
  DHS_BD_DATASET  dataset;
  DHS_BD_FRAME    dataFrame;
  unsigned long   dims[1];
  unsigned long   axisSize[2];
  unsigned long   origin[2];
  DHS_TAG         putTag;
  DHS_CMD_STATUS  sendStatus;
  DHS_BOOLEAN     dhsLastFlag;
  char            msg[80];
  unsigned long   axisDims[1];


  /* setup for new frame */
  /* get data label */
  dataLabel = dhsBdName (dhsConnection, &mydhsStatus);
  sprintf (label, "%s.0.0", dataLabel);
  printf ("label = %s, dhs status = %d\n", label, mydhsStatus);

  /* create new dataset */
  dims[0] = 2;
  axisDims[0] = 1;
  axisSize[0] = 128;  
  axisSize[1] = 128;
  origin[0] = 1;
  origin[1] = 1;
  printf ("Xlabel=%s Ylabel=%s\n", axisLabel[0], axisLabel[1]);

l=1;
for (k=0; k<nloop; k++) {

  l = l * (-1);

/* Create a dataset */

  printf ("\n\nCreating dataset %d\n", k);

  dataset = dhsBdDsNew (&mydhsStatus);
  printf ("After dhsBdDsNew, dhsStatus=%d\n",mydhsStatus);

  /* Mandatory dataset headers */

  dhsBdAttribAdd (dataset, "instrument", DHS_DT_STRING, 0, NULL, "agwps0", \
		  &mydhsStatus);

 if (k==0)
 {
  dhsBdAttribAdd (dataset, "telescope", DHS_DT_STRING, 0, NULL, "UKATC", \
		  &mydhsStatus);
  printf ("After first calls to dhsBdAttribAdd, dhsStatus=%d\n",mydhsStatus);
 }

  /* Create frame */
  dataFrame = dhsBdFrameNew (dataset, "dataArray", 0, DHS_DT_UINT16, 2, axisSize, \
			     (const void **) &pDhsData, &mydhsStatus);
  printf ("After dhsBdFrameNew, dhsStatus=%d\n",mydhsStatus);

  dhsBdAttribAdd (dataFrame, "origin", DHS_DT_INT32, 1, dims, origin, \
		  &mydhsStatus);
  dhsBdAttribAdd (dataFrame, "axisSize", DHS_DT_INT32, 1, dims, axisSize, \
		  &mydhsStatus);

 if (k==0)
 {

  /* add frame header info. */
  dhsBdAttribAdd (dataFrame, "dataType", DHS_DT_STRING, 0, NULL, "Electrons", \
		  &mydhsStatus);
  dhsBdAttribAdd (dataFrame, "frameName", DHS_DT_STRING, 0, NULL, "A&G QL Test", \
		  &mydhsStatus);
  dhsBdAttribAdd (dataFrame, "frameId", DHS_DT_STRING, 0, NULL, "0", \
		  &mydhsStatus);
  dhsBdAttribAdd (dataFrame, "axisLabel", DHS_DT_STRING, 1, dims, axisLabel, \
		  &mydhsStatus);
  printf ("After second calls to dhsBdAttribAdd, dhsStatus=%d\n",mydhsStatus);

 }

  /* Send data frame */

  printf ("Sending data frame %d...\n", k);

  ptr = pDhsData;
  for (i=0; i<128*128;i++)
  {
    *ptr = (unsigned short) (17000 + (i + k)*l/64);
	/* printf ("Data[%d %d %d] = %d\n", i, k, l, (int) *ptr); */
    ptr++;
  }

  if ( (k==0) || (k>=(nloop-1)) )
     dhsBdDsPrint (dataset, &mydhsStatus);

  /* send the data to the dhs */
  if ( k<(nloop-1) )
    dhsLastFlag = DHS_FALSE;
  else
    dhsLastFlag = DHS_TRUE;

  putTag = dhsBdPut (dhsConnection, label, DHS_BD_PT_DS_QL, dhsLastFlag, dataset, \
		     NULL, &mydhsStatus);
  printf ("After %s dhsBdPut, dhsStatus=%d\n",
           ((dhsLastFlag==DHS_TRUE)?"LAST":"intermediate"), mydhsStatus);

  /* wait for completion */
  dhsWait (1, &putTag, &mydhsStatus);
  printf ("After dhsWait, dhsStatus=%d\n",mydhsStatus);

  sendStatus = dhsStatus (putTag, &msg, &mydhsStatus);
  if (sendStatus != DHS_CS_DONE) {
    printf ("Failed to export the data!\n, %s\n", msg);
    printf ("sendStatus=%d, dhsStatus=%d\n",sendStatus, mydhsStatus);
    printf ("Message=%s\n", msg);
  }

  dhsTagFree (putTag, &mydhsStatus);
  printf ("After dhsTagFree, dhsStatus=%d\n",mydhsStatus);

  dhsBdDsFree (dataset, &mydhsStatus);
  printf ("After dhsBdDsFree, dhsStatus=%d\n",mydhsStatus);

} /* Next k */

  if (mydhsStatus != DHS_S_SUCCESS) {
    printf ("Bad dhs status %d \n", mydhsStatus);
    mydhsStatus = DHS_S_SUCCESS;
  }
}

void          slowlook(
  int             nloop
)
{

  int             i, j, k, l;
  float           localData[3][2];
  float          *pDhsData;
  float          *ptr;
  char           *dataLabel;
  char            label[50];
  char           *axisLabel[2]={ "None", "None"};
  DHS_BD_DATASET  dataset;
  DHS_BD_FRAME    dataFrame;
  unsigned long   dims[1];
  unsigned long   axisSize[2];
  unsigned long   origin[2];
/*  float           intData[3][2]; */
  DHS_TAG         putTag;
  DHS_CMD_STATUS  sendStatus;
  DHS_BOOLEAN     dhsLastFlag;
  char            msg[80];
  unsigned long   axisDims[1];

  for (i=0; i<3; i++) {
    for (j=0;j<2;j++) {
      localData[i][j] = 1.0;
    }
  }

l=1;
for (k=0; k<nloop; k++) {

  l = l * (-1);

  /* setup for new frame */
  /* get data label */
  dataLabel = dhsBdName (dhsConnection, &mydhsStatus);
  sprintf (label, "%s.0.0", dataLabel);
  printf ("label %d = %s, dhs status = %d\n", k, label, mydhsStatus);

  /* create new dataset */
  dims[0] = 2;
  axisDims[0] = 1;
  axisSize[0] = 3;  
  axisSize[1] = 2;
  origin[0] = 1;
  origin[1] = 1;
  printf ("Xlabel=%s Ylabel=%s\n", axisLabel[0], axisLabel[1]);

/* Create a dataset */

  printf ("\n\nCreating dataset %d\n", k);

  dataset = dhsBdDsNew (&mydhsStatus);
  printf ("After dhsBdDsNew, dhsStatus=%d\n",mydhsStatus);

  /* Mandatory dataset headers */

  dhsBdAttribAdd (dataset, "instrument", DHS_DT_STRING, 0, NULL, "agwps0", \
		  &mydhsStatus);

  dhsBdAttribAdd (dataset, "telescope", DHS_DT_STRING, 0, NULL, "UKATC", \
		  &mydhsStatus);
  printf ("After first calls to dhsBdAttribAdd, dhsStatus=%d\n",mydhsStatus);

  /* Create frame */
  dataFrame = dhsBdFrameNew (dataset, "dataArray", 0, DHS_DT_FLOAT, 2, axisSize, \
			     (const void **) &pDhsData, &mydhsStatus);
  printf ("After dhsBdFrameNew, dhsStatus=%d\n",mydhsStatus);

  dhsBdAttribAdd (dataFrame, "origin", DHS_DT_INT32, 1, dims, origin, \
		  &mydhsStatus);


  /* add frame header info. */
  dhsBdAttribAdd (dataFrame, "dataType", DHS_DT_STRING, 0, NULL, "Electrons", \
		  &mydhsStatus);
  dhsBdAttribAdd (dataFrame, "frameName", DHS_DT_STRING, 0, NULL, "A&G SL Test", \
		  &mydhsStatus);
  dhsBdAttribAdd (dataFrame, "frameId", DHS_DT_STRING, 0, NULL, "0", \
		  &mydhsStatus);
  dhsBdAttribAdd (dataFrame, "axisSize", DHS_DT_INT32, 1, dims, axisSize, \
		  &mydhsStatus);
  dhsBdAttribAdd (dataFrame, "axisLabel", DHS_DT_STRING, 1, dims, axisLabel, \
		  &mydhsStatus);
  printf ("After second calls to dhsBdAttribAdd, dhsStatus=%d\n",mydhsStatus);


  /* Send data frame */

  printf ("Sending data frame %d...\n", k);

  ptr = pDhsData;
  for (i=0; i<3*2;i++)
  {
    *ptr = (float) ((i + k)*l);
	printf ("Data[%d %d %d] = %f\n", i, k, l, *ptr);
    ptr++;
  }

  if ( (k==0) || (k>=(nloop-1)) )
     dhsBdDsPrint (dataset, &mydhsStatus);

  /* send the data to the dhs */

  putTag = dhsBdPut (dhsConnection, label, DHS_BD_PT_DS_QL, DHS_FALSE, dataset, \
		     NULL, &mydhsStatus);
  printf ("After dhsBdPut, dhsStatus=%d\n", mydhsStatus);

  /* wait for completion */
  dhsWait (1, &putTag, &mydhsStatus);
  printf ("After dhsWait, dhsStatus=%d\n",mydhsStatus);

  sendStatus = dhsStatus (putTag, &msg, &mydhsStatus);
  if (sendStatus != DHS_CS_DONE) {
    printf ("Failed to export the data!\n, %s\n", msg);
    printf ("sendStatus=%d, dhsStatus=%d\n",sendStatus, mydhsStatus);
    printf ("Message=%s\n", msg);
  }

  dhsTagFree (putTag, &mydhsStatus);
  printf ("After dhsTagFree, dhsStatus=%d\n",mydhsStatus);

  dhsBdDsFree (dataFrame, &mydhsStatus);
  printf ("After dhsBdDsFree of dataFrame, dhsStatus=%d\n",mydhsStatus);

  dhsBdDsFree (dataset, &mydhsStatus);
  printf ("After dhsBdDsFree of dataset, dhsStatus=%d\n",mydhsStatus);

} /* Next k */

  if (mydhsStatus != DHS_S_SUCCESS) {
    printf ("Bad dhs status %d \n", mydhsStatus);
    mydhsStatus = DHS_S_SUCCESS;
  }
}

void daqDhsErrorCallback (DHS_CONNECT connect, DHS_STATUS errorNum, \
			  DHS_ERR_LEVEL errorLev, char *msg, DHS_TAG tag, \
			  void *userData)
{
  logMsg ("In dhs error callback: errorNum=%d errorLev=%d msg=%s\n", errorNum, errorLev, msg, 0,0,0);
}

void daqDhsGetCallback ()
{
  logMsg ("In dhs get callback\n", 0,0,0,0,0,0);
}

void daqDhsPutCallback ()
{
  logMsg ("In dhs put callback\n", 0,0,0,0,0,0);
}

void             initDhs(
	const char *	server
)
{

  /* do dhs initialise things here... */
  mydhsStatus = DHS_S_SUCCESS;
  printf ("Begin dhsStatus %d \n", mydhsStatus);

  /* need to put a unique identifying name in here! */
  dhsInit ("agwps0", 16, &mydhsStatus);
  printf ("After init dhStatus %d \n", mydhsStatus);

  /* should I use callbacks? If so set them up here */
  dhsCallbackSet (DHS_CBT_ERROR, daqDhsErrorCallback, &mydhsStatus);
  printf ("After cb error dhs status %d \n", mydhsStatus);

/*
  dhsCallbackSet (DHS_CBT_GET, daqDhsGetCallback, &mydhsStatus);
  printf ("After cb get dhs status %d \n", mydhsStatus);
  dhsCallbackSet (DHS_CBT_PUT, daqDhsPutCallback, &mydhsStatus);
  printf ("After cbput dhsStatus %d \n", mydhsStatus);
*/

  /* connect to the server */
  printf ("Connecting to server %s...\n", server);
  dhsConnection = dhsConnect ("alba", server, NULL, \
			      &mydhsStatus);
  if (mydhsStatus != DHS_S_SUCCESS ) {
    logMsg ("Failed to connect to DHS server!\n", 0,0,0,0,0,0);
  }
  printf ("After connect dhsStatus %d \n", mydhsStatus);

  /* start dhs event loop */
  dhsEventLoop (DHS_ELT_THREADED, NULL, &mydhsStatus);
  printf ("After event loop dhs status %d \n", mydhsStatus);

}

void exitDhs()
{
  dhsExit( &mydhsStatus );
  printf ("After exit dhs status %d \n", mydhsStatus);
}
