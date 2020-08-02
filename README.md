# Invisible_Clock
 Harry Potter Movie Invisible Trick

# Invisible clock
# Required OpenCv build with Visual Studio(MSVC)

# Changes Required.
If you want to run with Webcam, Replace
   VideoCapture cap(url); -> VideoCapture cap(0); 	in line 70
   
2. If you want to get upper and lower HSV values to create masking image then Replace 
   "TRACT_BAR_MODE"	Preprocessor   -> #define TRACT_BAR_MODE 1 (you get access with track bar with thresholded image).
 
3. Once After Getting upper and Lower values , make changes according to your masking values
   #define LOW_HSV		Vec3b lowHsv(143,108,9)		// Lower Limit for Masking
   #define HIGH_HSV		Vec3b highHsv(179, 255, 136)	// Upper Limit for Masking

4. Run  the Project and check the output.


optional extra info : If you want to run with IP webcamera from mobile, download IP-WebCam from playstore and replace URL provided by application [cap(url)] 
