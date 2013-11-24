- On Windows:
	- Get VS 2012 Express from http://www.microsoft.com/en-us/download/confirmation.aspx?id=34673 (OpenCV does not support VS 2013 yet)
		- Settings: "Replace tabs by spaces" 
		- Create your own Makefile for other toolchains
	
- SmartVideo app:
	- Setup OpenCV (On Windows: Version 2.47, 64 bit)
		- On Windows: Close Visual Studio first!
		- Follow these four steps (http://docs.opencv.org/doc/tutorials/introduction/windows_install/windows_install.html):
			1. Download version 2.47 from http://opencv.org/downloads.html
			2. Make sure you have admin rights. Unpack the self-extracting archive.
			3. To finalize the installation go to the Set the OpenCV enviroment variable and add it to the systems path section.
				-> setx -m OPENCV_DIR <open-cv install path>\Build\x64\vc11
	- Run SmartVideo/smart-video-2013.sln (make sure, you have VS 2012)
	- Should work
	
- Viewer:
	- Run viewer by just executing: viewer/index.html
	- Chrome: 
		- Allow Chrome to read local files from files (in order to run the viewer without a web server)
			- http://stackoverflow.com/questions/18586921/how-to-launch-html-using-chrome-at-allow-file-access-from-files-mode
			- Apparently, there is a bug - So you might have to add both "--allow-file-access-from-files -allow-file-access-from-files" to start Chrome
	- IE & Firefox:
		- Sometimes work, sometimes don't
		- IE might need some changes in your security settings
	- Enable debug console while developing:
		- Ctrl + Shift + J in Chrome
		- F12 in IE

- Data:
	- The data directory is defined in "cfg/config.json"
	- Data set configurations are defined in "cfg/clips.json" (currently only supports image sequences, gotta fix that)
	- Setup:
		- Download data sets from:
			- http://wordpress-jodoin.dmi.usherb.ca/static/dataset/dataset.zip
			- http://burnbit.com/torrent/235660/SABS_Basic_rar
		- Extract to some folder <data-root>
			- I placed it in: ../data/
			- If you have different folder, update "cfg/config.json" correspondingly
		- If you want to add more data sets:
			1. Also extract them into that folder
			2. Run "ls > cfg/<datasetname>.txt" (to get all image file names)
			3. Add entry to cfg/clips.json
- TODO
	- a lot more...