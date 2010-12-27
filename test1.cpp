#include "stdafx.h"
#include "cv.h"
#include "highgui.h"
#include <ks.h>
#include <Ksproxy.h>
#include "LVUVCPublic.h"
#include <stdio.h>

#include "camcontrol.h"

#define RGB_MAX_VAL (255 / 3)
#define THRESH (7)
#define SHOW_FILTERED_IMG
// When using SAVE_IMG, the HDD write can sometimes stall the program
#define SAVE_IMG
//#define STEREO

#define BIN_WEIGHT (1/3.0)
#define K_MEANS_NUM_CLUSTERS (5)
#define K_MEANS_MAX_ITER (5)
#define TRACKINGHISTORY_SIZE (15)
#define MIN_NUM_SAMPLES_FOR_MOTION (20)
// TODO: These should be in proportion to cam resolution.
#define MOVEMENT_RADIUS_SQUARED (70*70)
#define MOVEMENT_CRITERIA_X (200)
#define MOVEMENT_CRITERIA_Y (200)
#define LOG_VERBOSE (0)
static const CvPoint TextPoint = {10,10};
static CvScalar color_tab[K_MEANS_NUM_CLUSTERS] = {CV_RGB(255,0,0), 
												CV_RGB(0,255,0), 
												CV_RGB(100,100,255), 
												CV_RGB(255,0,255),
												CV_RGB(255,255,0)};

#define LOG_DBG_(x) do { \
	if (LOG_VERBOSE) printf(x); } \
	while (0)

typedef struct {
	CvPoint2D32f HistPArr[K_MEANS_NUM_CLUSTERS];
	unsigned int Counters[K_MEANS_NUM_CLUSTERS];
	CvPoint2D32f MaxVals[K_MEANS_NUM_CLUSTERS];
	CvPoint2D32f MinVals[K_MEANS_NUM_CLUSTERS];
} TrackHist_t;

typedef enum {
	MOVED_LEFT,
	MOVED_RIGHT,
	MOVED_UP,
	MOVED_DOWN,
	MOVED_NO
} movement_t;

static int AbsDiff(void);
static int test2(void);
static int ApplyGreyThreshold(IplImage* const SrcImg_p, IplImage** const DstImage_pp);
static void FindImgCentreOfGravity(IplImage* const Img_p, CvPoint* const Point_p);
static double FindImgCentreOfGravityHelper(IplImage* const Img_p);
static int CreateBinaryImage(IplImage* const SrcImg_p, IplImage** const DstImage_pp);
static int FillPointMatrix(const IplImage* const SrcDstImg_p, CvMat* const points, unsigned int NumberOfSamples);
static int DrawCirclesInImage(IplImage* const DstImg_p, 
							  const CvMat* const points, 
							  const CvMat* const clusters, 
							  unsigned int NumberOfSamples);
static int CustKMeans( const CvMat* const SampleCoords_p, unsigned int cluster_count,
					  CvMat** const InCenterCoords_pp, CvTermCriteria termcrit, CvMat* const SampleToClassMap_p);
static int DetectMotion( TrackHist_t* const History_p, const CvMat* const CenterCoords_p);
static void UpdateMinAndMaxVals(TrackHist_t* const History_p, unsigned int idx, CvPoint2D32f pt);
static void ResetHistory(TrackHist_t* const History_p, unsigned int idx);
static int CheckDistanceToPrev(TrackHist_t* const History_p, unsigned int idx, CvPoint2D32f pt);
static movement_t CheckForMotion(TrackHist_t* const History_p, unsigned int idx);

#define BASE_FMT_STRING "C:\\Documents and Settings\\Ola\\Desktop\\OpenCVImages\\testbild%u.jpg"

// A Simple Camera Capture Framework
int main() 
{
  //return AbsDiff();  
	// Obtain the version number of the property set

	return test2();
}

static const char* Move2String(movement_t move)
{
	switch (move) {
	case MOVED_LEFT:
		return "LEFT";
	case MOVED_RIGHT:
		return "RIGHT";
	case MOVED_UP:
		return "UP";
	case MOVED_DOWN:
		return "DOWN";
	}
}

static int CheckDistanceToPrev(TrackHist_t* const History_p, unsigned int idx, CvPoint2D32f pt)
{
	if (History_p->HistPArr[idx].x == -1)
	{
		/* Previous value was invalid, just update and return */
		History_p->HistPArr[idx].x = pt.x;
		History_p->HistPArr[idx].y = pt.y;
		return 1;
	}

	/* Check if distance to previous point is within acceptable radius */
	CvPoint2D32f prev_pt; 
	prev_pt.x = History_p->HistPArr[idx].x;
	prev_pt.y = History_p->HistPArr[idx].y;

	if ((prev_pt.x-pt.x)*(prev_pt.x-pt.x)+(prev_pt.y-pt.y)*(prev_pt.y-pt.y)
		< MOVEMENT_RADIUS_SQUARED)
	{
		History_p->HistPArr[idx].x = pt.x;
		History_p->HistPArr[idx].y = pt.y;
		return 1;
	}

	/* Too large distance to previous point */
	return 0;
}

static void ResetHistory(TrackHist_t* const History_p, unsigned int idx)
{
	/* Reset prev point to invalid value */
	History_p->HistPArr[idx].x = -1;
	History_p->HistPArr[idx].y = -1;

	/* Reset Counter */
	History_p->Counters[idx] = 0;

	/* Reset max and min values */
	History_p->MaxVals[idx].x = FLT_MIN;
	History_p->MaxVals[idx].y = FLT_MIN;	
	History_p->MinVals[idx].x = FLT_MAX;
	History_p->MinVals[idx].y = FLT_MAX;
}

static movement_t CheckForMotion(TrackHist_t* const History_p, unsigned int idx)
{
	int XDiff = History_p->MaxVals[idx].x-History_p->MinVals[idx].x;
	int YDiff = History_p->MaxVals[idx].y-History_p->MinVals[idx].y;

	if (abs(XDiff) > MOVEMENT_CRITERIA_X)
	{
		if (History_p->HistPArr[idx].x > History_p->MinVals[idx].x)
		{
			return MOVED_LEFT; /* MIRROR! */
		}
		return MOVED_RIGHT;
	}

	if (abs(YDiff) > MOVEMENT_CRITERIA_Y)
	{
		if (History_p->HistPArr[idx].y > History_p->MinVals[idx].y)
		{
			return MOVED_DOWN; /* Origin in upper left corner */
		}
		return MOVED_UP;
	}

	return MOVED_NO;
}



	


static void UpdateMinAndMaxVals(TrackHist_t* const History_p, unsigned int idx, CvPoint2D32f pt)
{
	if (pt.x > History_p->MaxVals[idx].x)
	{
		History_p->MaxVals[idx].x = pt.x;
	}

	if (pt.y > History_p->MaxVals[idx].y)
	{
		History_p->MaxVals[idx].y = pt.y;
	}

	if (pt.x < History_p->MinVals[idx].x)
	{
		History_p->MinVals[idx].x = pt.x;
	}

	if (pt.y < History_p->MinVals[idx].y)
	{
		History_p->MinVals[idx].y = pt.y;
	}
}

static int DetectMotion( TrackHist_t* const History_p, const CvMat* const CenterCoords_p)
{
	CvPoint2D32f* CenterCoordsFloat_p = (CvPoint2D32f *) CenterCoords_p->data.fl;
	CvPoint2D32f pt;
	int i;
	int NumCenters = CenterCoords_p->rows;
	movement_t moved;

	LOG_DBG_(("DetectMotion\n"));

	/* First, add the CenterCoords to the tracking */
	for(i = 0; i < NumCenters; ++i)
	{
		pt = CenterCoordsFloat_p[i];

		/* Check if distance to previous point (if any) is OK and we should continue tracking this center */
		if (CheckDistanceToPrev(History_p, i, pt)) // TODO CheckDistanceToPrev(History_p, i, pt)
		{
			UpdateMinAndMaxVals(History_p, i, pt);

			if ((moved = CheckForMotion(History_p, i)) != MOVED_NO)
			{
				/* Motion detected */
				printf("Detected movement %s\n", Move2String(moved));
				/* Reset history for all centers and exit, disregard any other movements */
				for (int j = 0; j < NumCenters; ++j)
				{
					ResetHistory(History_p, j);
				}
				return 1;
			}
			else
			{
				History_p->Counters[i]++;
			}
		}
		else
		{
			ResetHistory(History_p, i);
		}
	}

	/* No motion detected */
	return 0;
}

static int CreateBinaryImage(IplImage* const SrcImg_p, IplImage** const DstImage_pp)
{
  IplImage* SrcImgRed_p   = NULL;
  IplImage* SrcImgBlue_p  = NULL;
  IplImage* SrcImgGreen_p = NULL;
  CvSize    Size;
  IplImage* DstImage_p = *DstImage_pp;
  int NumberOfNonZeroSamples = 0;
  CvScalar Sum;

  LOG_DBG_(("CreateBinaryImage\n"));

  Size.height = SrcImg_p->height;
  Size.width  = SrcImg_p->width;

  DstImage_p  = cvCreateImage(Size, SrcImg_p->depth, 1);
  SrcImgBlue_p  = cvCloneImage(DstImage_p );
  SrcImgGreen_p = cvCloneImage(DstImage_p );
  SrcImgRed_p = cvCloneImage(DstImage_p );

  cvSplit(SrcImg_p, SrcImgRed_p, SrcImgGreen_p, SrcImgBlue_p, NULL);

  cvAddWeighted(SrcImgRed_p, BIN_WEIGHT, SrcImgGreen_p, BIN_WEIGHT, 0, SrcImgGreen_p);
  cvAddWeighted(SrcImgGreen_p, 0, SrcImgBlue_p, BIN_WEIGHT, 0, DstImage_p);

  cvThreshold(DstImage_p, DstImage_p, THRESH, 1, CV_THRESH_BINARY);

  cvReleaseImage(&SrcImgRed_p);
  cvReleaseImage(&SrcImgGreen_p);
  cvReleaseImage(&SrcImgBlue_p);

  *DstImage_pp = DstImage_p;

  Sum = cvSum(DstImage_p);
  NumberOfNonZeroSamples = (int) Sum.val[0];

  return (int) cvSum(DstImage_p).val[0];
}

static int DrawCirclesInImage(IplImage* const DstImg_p, 
							  const CvMat* const points,
							  const CvMat* const clusters,
							  unsigned int NumberOfSamples)
{
	int i;

	LOG_DBG_(("DrawCirclesInImage\n"));

	for (i=0; i < NumberOfSamples; ++i)
	{
		CvPoint2D32f pt = ((CvPoint2D32f*)points->data.fl)[i];
		int cluster_idx;
		if (clusters != NULL)
		{
			cluster_idx = clusters->data.i[i];
			cvCircle( DstImg_p,
                  cvPointFrom32f(pt),
                  1,
                  color_tab[cluster_idx],
			      CV_FILLED );
		}
		else
		{
			cluster_idx = i;
			cvCircle( DstImg_p,
                  cvPointFrom32f(pt),
                  4,
                  color_tab[cluster_idx],
			      CV_FILLED );
		}
        /*cvCircle( DstImg_p,
                  cvPointFrom32f(pt),
                  1,
                  color_tab[cluster_idx],
			      CV_FILLED );*/
		
    }
	return 1;
}



static void FindImgCentreOfGravity(IplImage* const Img_p, CvPoint* const Point_p)
{
	CvSize ImgSize = cvGetSize(Img_p);
	CvSize TranspSize = {ImgSize.height, ImgSize.width};
	IplImage* TransposedImg_p = cvCreateImage(TranspSize, 8, 1);

	cvTranspose(Img_p, TransposedImg_p);

	Point_p->y = (int) FindImgCentreOfGravityHelper(Img_p);
	Point_p->x = (int) FindImgCentreOfGravityHelper(TransposedImg_p);

	cvReleaseImage(&TransposedImg_p);

}

static double FindImgCentreOfGravityHelper(IplImage* const Img_p)
{
	unsigned char* RawData_p;
	int StepSize;
	CvSize	     ImgSize;
	int x;
	int y;
	double RowSumTotal = 0;
	double RowSumMult  = 0;
	
	cvGetRawData( Img_p, &RawData_p, &StepSize, &ImgSize );

	

	for( y = 0; y < ImgSize.height; ++y, RawData_p += StepSize )
	{
		double RowSum = 0;
		for( x = 0; x < ImgSize.width; ++x )
		{
			RowSum += RawData_p[x];  
		}
		RowSumTotal += RowSum;
		RowSumMult  += y * RowSum;
	}
	
	if (RowSumTotal != 0)
	{
		return (RowSumMult / (double) RowSumTotal);
	}

	return 0;
}

static int FillPointMatrix(const IplImage* const SrcImg_p, CvMat* const points, unsigned int NumberOfSamples)
{
	unsigned char* RawData_p;
	int StepSize;
	CvSize	     ImgSize;
	int x;
	int y;
	unsigned int sample_counter = 0;
	CvPoint2D32f* sample_ptr = (CvPoint2D32f *) points->data.fl;

	LOG_DBG_(("FillPointMatrix\n"));

	cvGetRawData( SrcImg_p, &RawData_p, &StepSize, &ImgSize );

	for( y = 0; y < ImgSize.height; ++y, RawData_p += StepSize )
	{
		for( x = 0; x < ImgSize.width; ++x )
		{
			if (RawData_p[x] != 0)
			{
				if (sample_counter >= NumberOfSamples)
				{
					*((int *)0) = 0; //Dump, this shouldn't happen
				}
				sample_ptr[sample_counter++] = cvPoint2D32f(x,y);
			}
		}
	}

	return sample_counter; //Should be NumberOfSamples
}






static int ApplyGreyThreshold(IplImage* const SrcImg_p, IplImage** const DstImage_pp)
{
  IplImage* SrcImgRed_p   = NULL;
  IplImage* SrcImgBlue_p  = NULL;
  IplImage* SrcImgGreen_p = NULL;
  IplImage* DstImg_p      = *DstImage_pp;
  CvSize    Size;

  Size.height = SrcImg_p->height;
  Size.width  = SrcImg_p->width;

  SrcImgRed_p   = cvCreateImage(Size, SrcImg_p->depth, 1);
  SrcImgBlue_p  = cvCloneImage(SrcImgRed_p );
  SrcImgGreen_p = cvCloneImage(SrcImgRed_p );
  DstImg_p      = cvCloneImage( SrcImgRed_p );

  cvSplit(SrcImg_p, SrcImgRed_p, SrcImgGreen_p, SrcImgBlue_p, NULL);

  cvThreshold(SrcImgRed_p, SrcImgRed_p, THRESH, RGB_MAX_VAL, CV_THRESH_BINARY);
  cvThreshold(SrcImgGreen_p, SrcImgGreen_p, THRESH, RGB_MAX_VAL, CV_THRESH_BINARY);
  cvThreshold(SrcImgBlue_p, SrcImgBlue_p, THRESH, RGB_MAX_VAL, CV_THRESH_BINARY);

  cvAdd(SrcImgRed_p, SrcImgGreen_p, SrcImgGreen_p, 0);
  cvAdd(SrcImgGreen_p, SrcImgBlue_p, DstImg_p, 0);

  cvReleaseImage(&SrcImgRed_p);
  cvReleaseImage(&SrcImgGreen_p);
  cvReleaseImage(&SrcImgBlue_p);

  *DstImage_pp = DstImg_p;

  return 1;
}

static int test2(void)
{
           CvCapture*   capture             = cvCaptureFromCAM( CV_CAP_ANY );
#ifdef STEREO
           CvCapture*   capture2            = cvCaptureFromCAM(1);
#endif
		   IplImage*    CurrFrame_p         = NULL;
		   IplImage*    CurrFrame2_p        = NULL;
           IplImage*    PrevFrame_p         = NULL;
           IplImage*    DiffFrame_p         = NULL;
           IplImage*    GrayScaleDiff_p     = NULL;
		   CvMat*		InCenterCoords_p	= NULL;
		   IplImage*    BlackImage_p		= NULL;
           char*        ImageLocator_p      = (char *) malloc(200);
		   TrackHist_t  TrackingHistory;
  unsigned int          ImageCount          = 1;
		   double		Current_FPS;
		   int			i;


  if( !capture ) {
    fprintf( stderr, "ERROR: capture is NULL \n" );
    getchar();
    return -1;
  }
	
#ifdef STEREO
  if( !capture2 ) {
    fprintf( stderr, "ERROR: capture2 is NULL \n" );
    getchar();
    return -1;
  }
#endif

  //enum_devices();

  /* Initiate Tracking History array */
  for (i = 0; i < K_MEANS_NUM_CLUSTERS; ++i)
  {
	  ResetHistory(&TrackingHistory, i);
  }


#if 0
  (void) cvSetCaptureProperty(capture, KSPROPERTY_CAMERACONTROL_EXPOSURE
  (void) cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH_HEIGHT, 320x240);
  (void) cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, 320);
  (void) cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, 240);
  (void) cvSetCaptureProperty(capture, CV_CAP_PROP_FPS, 3);
#endif

  // Create a window in which the captured images will be presented
  cvNamedWindow( "mywindow", CV_WINDOW_AUTOSIZE );
#ifdef SHOW_FILTERED_IMG
  cvNamedWindow( "mywindow2", CV_WINDOW_AUTOSIZE );
#endif
  CurrFrame_p  = cvQueryFrame( capture );
#ifdef STEREO
  CurrFrame2_p = cvQueryFrame( capture2 );
#endif
  BlackImage_p = cvCloneImage(CurrFrame_p);
  cvSetZero(BlackImage_p);
  if( !CurrFrame_p ) 
  {
    fprintf( stderr, "ERROR: frame is null...\n" );
    return -1;
  }

#ifdef SAVE_IMG
  sprintf(ImageLocator_p, BASE_FMT_STRING, ImageCount);
  cvSaveImage(ImageLocator_p, CurrFrame_p);
#endif

  printf("Starting loop!\n");

  // Show the image captured from the camera in the window and repeat
  while( 1 ) {
    // Get one frame
	CvPoint cvPoint;
	IplImage* ImageToShow_p = NULL;
	/* FOR K-MEANS calculations */ 
	IplImage* BinaryImage_p = NULL;
	CvMat* points = NULL;
	//CvMat* clusters = NULL;
	int BinarySampleCount;
	
	PrevFrame_p = cvCloneImage(CurrFrame_p);
	//Current_FPS = cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
	printf("query frame\n");
	CurrFrame_p = cvQueryFrame( capture );
	ImageToShow_p = cvCloneImage(CurrFrame_p);
	if (!CurrFrame_p) {
		printf("BAJS OCKSÅ!\n");
		return -1;
	}
	cvShowImage( "mywindow", ImageToShow_p );
	cvReleaseImage(&ImageToShow_p);
	continue;
#ifdef STEREO
	CurrFrame2_p = cvQueryFrame( capture2 );
#endif
	//copy CurrFrame for displaying
	ImageToShow_p = cvCloneImage(CurrFrame_p);

    if( !CurrFrame_p ) {
      fprintf( stderr, "ERROR: frame is null...\n" );
      getchar();
      break;
    }
	
	DiffFrame_p = cvCloneImage( CurrFrame_p );

	cvAbsDiff(CurrFrame_p, PrevFrame_p, DiffFrame_p);
	
	//(void) ApplyGreyThreshold(DiffFrame_p, &GrayScaleDiff_p);

	BinarySampleCount = CreateBinaryImage(DiffFrame_p, &BinaryImage_p);

	if (BinarySampleCount > MIN_NUM_SAMPLES_FOR_MOTION)
	{
		CvMat* SamplesToClassMap = cvCreateMat(BinarySampleCount, 1, CV_32SC1);

		/* Create Point and cluster matrices for kmeans algorithm */
		points = cvCreateMat( BinarySampleCount, 1, CV_32FC2 ); // 32 bit float, 2 channels
		//clusters = cvCreateMat( BinarySampleCount, 1, CV_32SC1 ); // 32 bit signed, 1 channel

		/* Iterate and fill the points matrix with values */
		(void) FillPointMatrix(BinaryImage_p, points, BinarySampleCount);
		CustKMeans(points, K_MEANS_NUM_CLUSTERS, &InCenterCoords_p, cvTermCriteria(CV_TERMCRIT_ITER, K_MEANS_MAX_ITER, 0) , SamplesToClassMap);
		
		/* Draw points marking cluster centers */
		(void) DrawCirclesInImage(ImageToShow_p, InCenterCoords_p, NULL, K_MEANS_NUM_CLUSTERS);
		
		/* Draw all the points in all the clusters */
		(void) DrawCirclesInImage(BlackImage_p, points, SamplesToClassMap, BinarySampleCount);
		
		/* Check if there was motion and update tracking history */
		(void) DetectMotion(&TrackingHistory, InCenterCoords_p);
		
		cvReleaseMat(&points);
		cvReleaseMat(&SamplesToClassMap);
	}
	


#if 0
	FindImgCentreOfGravity(GrayScaleDiff_p, &cvPoint);

	cvCircle( ImageToShow_p, cvPoint, 10, CV_RGB( 255, 0, 0 ));
	cvCircle( ImageToShow_p, cvPoint, 8, CV_RGB( 255, 255, 0 ));
	cvCircle( ImageToShow_p, cvPoint, 6, CV_RGB( 255, 0, 255 ));
	cvCircle( ImageToShow_p, cvPoint, 4, CV_RGB( 0, 0, 0 ));
#endif
	//cvPutText(ImageToShow_p, "Hello", TextPoint, NULL, CV_RGB( 0, 255, 255 ));  
	LOG_DBG_(("Before ShowImage\n"));
	cvShowImage( "mywindow", ImageToShow_p );
	LOG_DBG_(("After ShowImage\n"));
#ifdef SHOW_FILTERED_IMG
	//cvShowImage( "mywindow2", BlackImage_p );
#ifdef STEREO
	cvShowImage( "mywindow2", CurrFrame2_p );
#endif
	cvSetZero(BlackImage_p);
#endif
	LOG_DBG_(("Before SaveImage\n"));
#ifdef SAVE_IMG
	ImageCount++;
	sprintf(ImageLocator_p, BASE_FMT_STRING, ImageCount);
    cvSaveImage(ImageLocator_p, CurrFrame_p);
#endif
	LOG_DBG_(("After SaveImage\n"));
    // Do not release the frame!
	
	// But release temporary diff frame
	cvReleaseImage(&DiffFrame_p);
	cvReleaseImage(&PrevFrame_p);
	//cvReleaseImage(&GrayScaleDiff_p);
	cvReleaseImage(&BinaryImage_p);
	cvReleaseImage(&ImageToShow_p);
	
    //If ESC key pressed, Key=0x10001B under OpenCV 0.9.7(linux version),
    //remove higher bits using AND operator

	int key = cvWaitKey(1);

    if( (key & 255) == 27 ) break;
	if ((key & 0xFF) == 'w' ) { printf("inc exposure!\n"); inc_exposure(0); inc_exposure(1);}
	if ((key & 0xFF) == 's' ) { printf("dec exposure!\n"); dec_exposure(0); dec_exposure(1); }
	if ((key & 0xFF) == 'a' ) { printf("auto exposure!\n"); auto_exposure(0); auto_exposure(1); }
	if ((key & 0xFF) == 'l' ) { printf("lock framerate!\n"); lock_framerate(0); lock_framerate(1);}
	if ((key & 0xFF) == 'u' ) { printf("unlock framerate!\n"); unlock_framerate(0); lock_framerate(1);}
  }

  // Release the capture device housekeeping
  cvReleaseImage(&BlackImage_p);
  cvReleaseMat( &InCenterCoords_p );
  cvReleaseCapture( &capture );
  cvDestroyWindow( "mywindow" );
#ifdef SHOW_FILTERED_IMG
  cvDestroyWindow( "mywindow2" );
#endif
  free(ImageLocator_p);
  return 0;
}

static int AbsDiff(void)
{
  CvCapture* capture     = cvCaptureFromCAM( CV_CAP_ANY );
  IplImage*  CurrFrame_p = NULL;
  IplImage*  PrevFrame_p = NULL;
  IplImage*  DiffFrame_p = NULL;

  if( !capture ) {
    fprintf( stderr, "ERROR: capture is NULL \n" );
    getchar();
    return -1;
  }

  // Create a window in which the captured images will be presented
  cvNamedWindow( "mywindow", CV_WINDOW_AUTOSIZE );

  CurrFrame_p = cvQueryFrame( capture );

  if( !CurrFrame_p ) 
  {
    fprintf( stderr, "ERROR: frame is null...\n" );
    return -1;
  }

  // Show the image captured from the camera in the window and repeat
  while( 1 ) {
    // Get one frame
    PrevFrame_p = cvCloneImage(CurrFrame_p);

	CurrFrame_p = cvQueryFrame( capture );

    if( !CurrFrame_p ) {
      fprintf( stderr, "ERROR: frame is null...\n" );
      getchar();
      break;
    }

	DiffFrame_p = cvCloneImage( CurrFrame_p );

	cvAbsDiff(CurrFrame_p, PrevFrame_p, DiffFrame_p);

	cvShowImage( "mywindow", DiffFrame_p );

	CurrFrame_p = cvQueryFrame( capture );
    // Do not release the frame!
	
	// But release temporary diff frame
	cvReleaseImage(&DiffFrame_p);
	cvReleaseImage(&PrevFrame_p);

    //If ESC key pressed, Key=0x10001B under OpenCV 0.9.7(linux version),
    //remove higher bits using AND operator
    if( (cvWaitKey(10) & 255) == 27 ) break;
  }

  // Release the capture device housekeeping
  cvReleaseCapture( &capture );
  cvDestroyWindow( "mywindow" );
  return 0;
}

static int CustKMeans( const CvMat* const SampleCoords_p, unsigned int cluster_count,
					  CvMat** const InCenterCoords_pp, CvTermCriteria termcrit , CvMat* const SampleToClassMap_p)
{
	CvMat* CenterCoords_p = *InCenterCoords_pp;
	unsigned int * NumSamplesPerCenter_p;
	CvMat* CenterCumSum_p;
    int i, j, sample_count;
    int iter;

	LOG_DBG_(("CustKMeans\n"));

	sample_count = SampleCoords_p->rows;
	
	/* Force dump if less samples than clusters, should be checked elsewhere */
	if (sample_count < cluster_count)
	{
		*((int *)0) = 0;
	}

	/* Create arrays holding SamplesCounter and cumulative sum of samplecoordinates */
	NumSamplesPerCenter_p = (unsigned int*)malloc(cluster_count * sizeof(*NumSamplesPerCenter_p));
	CenterCumSum_p = cvCreateMat(cluster_count, 1, CV_32FC2);

	/* Create matrix holding the current cluster index for each sample */
	

	/* If no initial centers provided, use the first samples in the input array as centers */
	if (CenterCoords_p == NULL)
	{
		CvPoint2D32f * SamplesCoordsFloat_p = ((CvPoint2D32f*)SampleCoords_p->data.fl);
		CvPoint2D32f * CenterCoordsFloat_p;

		CenterCoords_p = cvCreateMat( cluster_count, 1, CV_32FC2 );
		CenterCoordsFloat_p = ((CvPoint2D32f *)CenterCoords_p->data.fl);

		for (i = 0; i < cluster_count; i++)
		{
			CenterCoordsFloat_p[i] = SamplesCoordsFloat_p[i];
		}
	}
	
	/* Algorithm starting point, run it a maximum of
	   max_iter times, or until convergence */
    for( iter = 0; iter < termcrit.max_iter; iter++ )
    {
        CvPoint2D32f * SamplesCoordsFloat_p = ((CvPoint2D32f*)SampleCoords_p->data.fl);
		CvPoint2D32f * CenterCoordsFloat_p = ((CvPoint2D32f*)CenterCoords_p->data.fl);
		CvPoint2D32f * CenterCumSumCoordsFloat_p = ((CvPoint2D32f*)CenterCumSum_p->data.fl);
		int k;

		cvSetZero(CenterCumSum_p);
		memset(NumSamplesPerCenter_p, 0, cluster_count * sizeof(*NumSamplesPerCenter_p));
		
		/* Check all samples for the closest center */
        for( i = 0; i < sample_count; i++ )
        {
			float min_dist = FLT_MAX;
			int cluster_idx = 0;
			CvPoint2D32f CurrentSampleCoords = SamplesCoordsFloat_p[i];

			for (j = 0; j < cluster_count; j++)
			{
				CvPoint2D32f CurrentCenterCoords = CenterCoordsFloat_p[j];
				/* Calculate the square euclidian distance between the current sample and the current center */
				float dist = ((CurrentSampleCoords.x-CurrentCenterCoords.x) * 
					(CurrentSampleCoords.x-CurrentCenterCoords.x)) +
					((CurrentSampleCoords.y-CurrentCenterCoords.y) * 
					(CurrentSampleCoords.y-CurrentCenterCoords.y));
				if (dist < min_dist)
				{
					min_dist = dist;
					cluster_idx = j;
				}
			}
			
			/* Assign the resulting index to the Label Matrix */
			if (min_dist == FLT_MAX)
			{
				*((int *)0) = 0;
			}

			SampleToClassMap_p->data.i[i] = cluster_idx;

			/* Increment the count for the chosen cluster */
			NumSamplesPerCenter_p[cluster_idx]++;
			/* Add to cumulative sum of points belonging to the chosen cluster */
			CenterCumSumCoordsFloat_p[cluster_idx].x += SamplesCoordsFloat_p[i].x;
			CenterCumSumCoordsFloat_p[cluster_idx].y += SamplesCoordsFloat_p[i].y;
		}

		/* Calculate the new center values */
		for (k = 0; k < cluster_count; ++k)
		{
			unsigned int NumSamplesForThisCenter = NumSamplesPerCenter_p[k];

		    if (NumSamplesForThisCenter != 0)
			{
				CenterCoordsFloat_p[k].x = CenterCumSumCoordsFloat_p[k].x / NumSamplesForThisCenter;
				CenterCoordsFloat_p[k].y = CenterCumSumCoordsFloat_p[k].y / NumSamplesForThisCenter;
			}
			else /* We need to choose the point closest to old center */
			{
				//TODO: placeholder, for now we just use the old value, is this good?
			}
		}



	}
	/* Assign the center points output.*/
	*InCenterCoords_pp = CenterCoords_p; 
cleanup:

	free(NumSamplesPerCenter_p);
	cvReleaseMat(&CenterCumSum_p);

	return 1;
}
