function checkCompatability()
{
	// nothing to do right now
}

// Places a viewer in every class="viewer" element.
// If cfgFiles are not given, will determine config file from data-cfgFile attribute.
function loadViewers(cfgDir, cfgFiles)
{
	// pre-reqs
	checkCompatability();
	
	// create viewers array
	window.viewers = [];
	
	// add all viewers from the page to the array
	var viewerElements = $('.viewer');
	for (var i=0; i< viewerElements.length; i++) {	
		var cfgFile;
		var elem = viewerElements[i];
		if (isSet(cfgFiles)) {
			cfgFile = cfgFiles[i];
		}
		else if (!isSet(cfgFile = $(elem).data("viewercfg"))) {
			assert(false && "data-viewercfg attribute is not set on viewer");
		}
		
		assert(isSet(cfgFile));
		addViewer(elem, cfgDir, cfgFile, i);
	}
}

// Create and add new viewer to list of existing viewers
function addViewer(elem, cfgDir, cfgFile, id)
{
	window.viewers[id] = createViewer(elem, cfgDir, cfgFile, id);
}

// Return the viewer that contains the given element or null (if not in viewer)
function getContainingViewer(elem)
{
	return elem.closest(".viewer")[0];
}

// "JS-style OOP"
// see: http://stackoverflow.com/questions/1114024/constructors-in-javascript-objects
var _ViewerClassDef = (function () {
	var ctor = function(viewerId)
	{	
		// public members
		this.viewerId = viewerId;
		this.playTimer = null;
		this.startTime = 0;
		this.frames = [];
		
		// NOTE: Don't call any methods on viewer here, since it is not fully constructed yet!
		// Instead, we call setup() after the extend() function has returned.
	};
	
	ctor.prototype = {
		
		// ############################################################################################################
		// getters & setters
		
		isViewer : function() { return true; },
		
		isReady : function() { return this.entry || false; },
		
		isPlaying : function()
		{
			return this.playTimer != null;
		},
		
		// whether the current clip has reached it's last frame
		isEOF : function()
		{
			return this.entry.currentFrame >= this.frames.length-1;
		},
		
		// return the time per frame in milliseconds
		getFrameTime : function()
		{
			return Math.round(1000 / this.entry.fps);
		},
		
		// the time that should pass between displaying two different frames (caps at 30ms to reduce load)
		getPlaybackInterval : function()
		{
			return Math.max(this.getFrameTime(), 30);		// don't update more often than every 30ms
		},
		
		// Returns the frame index at the given ratio within the clip, where ratio=0 maps to the first and ratio=1 maps to the last frame.
		getFrameAtRatio : function(ratio)
		{
			return Math.min(this.entry.nFrames-1, Math.max(0, Math.round(ratio * this.entry.nFrames)));
		},
		
		getFrameWidth : function()
		{
			if (this.frames.length == 0) return 0;
			
			var firstFrame = this.frames[0];
			return firstFrame[0].width;				// return width of image (not the jQuery object)
		},
		
		getFrameHeight : function()
		{
			if (this.frames.length == 0) return 0;
			
			var firstFrame = this.frames[0];
			return firstFrame[0].height;				// return height of image (not the jQuery object)
		},
		
		
		// Directory and path information
		
		getConfigPath : function() { return concatPath(this.cfgDir, this.cfgFile); },
		
		getClipListPath : function() { return concatPath(this.cfgDir, this.clipinfoDir, this.clipListFile); },
		
		getFrameListPath : function(entry) { return concatPath(this.cfgDir, this.clipinfoDir, entry.frameFile); },
		
		getFrameWeightPath : function(entry) { return concatPath(this.cfgDir, this.clipinfoDir, entry.weightFile); },
		
		getFrameFileDir : function(entry) { return concatPath(this.cfgDir, this.dataDir, entry.baseDir); },
		
		// ############################################################################################################
		// setup
		
		// attach viewer functionality to viewer buttons
		setup : function()
		{
			viewer = this;
			this.viewerElem = $(this);
			  
			 // make sure, config is properly setup
			assert(typeof this.cfgFile !== "undefined");			// can't live without this guy
			
			// GUI setup
			(function(viewer) {
				viewer.viewerElem.on("clipLoaded", function(evt, entry) { viewer.onClipLoaded(entry); });
				assert(viewer.viewerElem.find("#frame").size() > 0);		// can't live without this guy
				viewer.frameCont = viewer.viewerElem.find("#frame").first();
				
				// play button
				viewer.playButton = viewer.viewerElem.find( "#play" );
				viewer.playButton.button()
				  .mousedown(function( event ) {
					viewer.togglePlay();
				  });
				  
				// stop button
				viewer.stopButton = viewer.viewerElem.find( "#stop" );	
				viewer.stopButton.button()
				  .mousedown(function( event ) {
					viewer.stop();
				  });
		
				// "faster" button
				viewer.fpsIncButton = viewer.viewerElem.find("#speed-up");
				viewer.fpsIncButton.button().mousedown(function(evt) {
					if (viewer != null && viewer.isReady())
					{
						viewer.setFPS(viewer.entry, viewer.entry.fps + window.viewerFPSDelta);
					}
				});
				
				// "slower" button
				viewer.fpsDecButton = viewer.viewerElem.find("#speed-down");
				viewer.fpsDecButton.button().mousedown(function(evt) {
					if (viewer != null && viewer.isReady())
					{
						viewer.setFPS(viewer.entry, viewer.entry.fps - window.viewerFPSDelta);
					}
				});
				
				viewer.frameInfoLabel = viewer.viewerElem.find("#frameInfo");
				
				// see: http://www.jeasyui.com/documentation/combobox.php
				viewer.clipList = viewer.viewerElem.find("#clipList");
				//viewer.clipList.combobox()
				viewer.clipList
					.change(function(e) { 
						var selected = viewer.clipList.children(':selected');
						var clipName = viewer.clipList.val();
						if (clipName in viewer.entries)
						{
							// load clip
							viewer.loadClip(viewer.entries[clipName]);
						}
					});
				viewer.statusLabel = viewer.viewerElem.find("#status");
				viewer.bar = viewer.viewerElem.find("#bar");
				viewer.bar.progressbar().mousedown(function ( event ) {
					// seek to relative position
					var offset = viewer.bar.offset(); 
					var relX = event.pageX - offset.left;
					var maxX = viewer.bar.width();
					viewer.seek(relX / maxX);
				});
				
				$.getJSON(viewer.getConfigPath())
					.fail(function( jqxhr, textStatus, error ) {
						var msg = "[" + textStatus + "] Failed to load config from file \"" + viewer.getConfigPath() + "\": " + error;
						console.warn(msg);		// warn dev
						viewer.onFail(msg);		// warn user
					})
					.done(function( cfg ) {
						// read clip list from file
						assert(isSet(cfg.dataDir) && isSet(cfg.clipFile));
						viewer.dataDir = cfg.dataDir;
						viewer.clipListFile = cfg.clipFile;
						viewer.clipinfoDir = cfg.clipDir;
						
						$.getJSON(viewer.getClipListPath())
							.fail(function( jqxhr, textStatus, error ) {
								var msg = "[" + textStatus + "] Failed to load clip list from file \"" + viewer.getClipListPath() + "\": " + error;
								console.warn(msg);		// warn dev
								viewer.onFail(msg);		// warn user
							})
							.done(function( entries ) {
								//logObject( "JSON Data", entries);
								viewer.entries = entries;
								viewer.clipList.empty();
								
								// iterate over all entries
								var first = {};
								var i = 0;
								var listData = [];
								for (var key in entries)
								{
									// add some more information to entry objects
									var entry = entries[key];
									entry.index = i;
									entry.name = key;
									entry.currentFrame = entry.startFrame;
									entry.baseDir = viewer.getFrameFileDir(entry);	// update base dir
								
									// add toString method
									entry.toString = function()
									{
										return this.name + " (FPS: " + this.fps + ")";
									};
									
									// add to clip list
									viewer.clipList.append(new Option(entry, entry.name));
									i++;
								}
								
								// run the first clip
								//logObject("entries", getFirstProperty(viewer.entries));
								
								viewer.loadClip(getFirstProperty(viewer.entries));
						});	// done reading clip list file
				}); // done reading config file
			})(viewer);
		},
		
		// load clip to player
		loadClip : function(entry)
		{
			viewer = this;
			viewer.stop();
			viewer.frames = [];
			
			// load frame names from file
			(function(viewer) {
				assert(entry.frameFile != undefined);		// can't live without this guy
				$.get(viewer.getFrameListPath(entry))
					.fail(function( jqxhr, textStatus ) {
						var framePath = viewer.getFrameListPath(entry);
						console.warn("Could not read file: " + framePath + " (" + textStatus + ")");		// warn dev
						viewer.onFail("Failed to load frame file: " + framePath);	// warn user
					})
					.done(function( content ) {
						// split text into lines
						lines = content.replace(/\r\n/g, "\n").replace(/\r/g, "\n").split("\n");
						entry.nFrames = lines.length;
						//console.log(lines);
						
						var frameIdx = 0;
						var frameResponseCount = 0;
						
						// add one frame per line
						for (var lineIdx in lines) {
							var frameName = lines[lineIdx].trim();
							if (frameName.length == 0 || frameName[0] == "#") {
								// ignore empty lines and comments
								--entry.nFrames;
								continue;
							}
							
							var frameElement = $(document.createElement("img"));
							frameElement.fileName = concatPath(entry.baseDir, frameName);
							(function(frameElement) {
								var onFrameResponse = function() {
									if (frameElement.index == entry.nFrames-1) {
										// we are done loading this clip's frame data
										// now load the weights
										viewer.entry = entry;
										entry.weights = [];
										if (typeof entry.weightFile !== "undefined") {
											$.get(viewer.getFrameWeightPath(entry))
												.fail(function( jqxhr, textStatus ) {
													if (entry != viewer.entry) return;
													
													console.warn("Could not read weights file: " + viewer.getFrameWeightPath(entry) + " (" + textStatus + ")");		// warn dev
													viewer.viewerElem.trigger("clipLoaded", [entry]);
												})
												.done(function( content ) {
													if (entry != viewer.entry) return;
													
													// convert array of lines to array of floats
													lines = content.replace(/\r\n/g, "\n").replace(/\r/g, "\n").split("\n");
													entry.weights = lines.map(parseFloat);
													viewer.viewerElem.trigger("clipLoaded", [entry]);
												});
										}
										else {
											viewer.viewerElem.trigger("clipLoaded", [entry]);
										}
									}
								};

								frameElement.attr("src", frameElement.fileName);
								frameElement.addClass("viewerFrame");
								frameElement.index = frameIdx;
								frameElement.toString = function()
								{
									return entry.name + "/#" + frameIdx + " (" + frameElement.fileName + ")";
								};
								viewer.frames[frameIdx++] = frameElement;
								frameElement.load(function()
								{
									//console.log("Frame loaded: #" + frameIdx);
									viewer.statusLabel.text("Loaded frame " + (frameElement.index+1) + " of " + entry.nFrames);
									onFrameResponse();
								});
								frameElement.error(function()
								{
									var statusText = "Could not load frame: " + frameElement.fileName;
									viewer.statusLabel.text(statusText);
									//console.warn(statusText);			// browsers usually report this error anyway
									//--entry.nFrames;
									onFrameResponse();
								});
							})(frameElement);
						}
					});
			})(viewer);
		},
		
		// we are done loading the clip: Play
		onClipLoaded : function(entry)
		{
			assert(entry.nFrames);
			this.statusLabel.text("Finished loading clip: " + entry);
			if (this.frames.length > 0)
			{
				this.bar.progressbar();
				this.play();
			}
		},
		
		
		// ############################################################################################################
		// clip navigation
		
		// set the current frame
		setFrame : function(newFrameIdx, updateOnly)
		{	
			if (this.frames.length == 0) return;
			
			if (newFrameIdx >= this.frames.length-1)
			{
				newFrameIdx = Math.min(newFrameIdx, this.frames.length-1);
			}
				
			// check if the frame exists
			if (newFrameIdx != this.entry.currentFrame)
			{
				this.entry.currentFrame = newFrameIdx;
				var newFrame = this.frames[newFrameIdx];
				assert(frame != null);
				
				// update UI:
				var previousFrame = this.frameCont.find(".viewerFrame");
				if (previousFrame.length > 0)
				{
					// replace previous frame
					var parent = previousFrame.parent()[0];
					parent.removeChild(previousFrame[0]);
					parent.appendChild(newFrame[0]);
					
					//previousFrame.replaceWith(newFrame);		// jQuery way of doing it
				}
				else
				{
					this.frameCont.append(newFrame);			// append frame to container
				}
				newFrame.center(this.frameCont);
				var millisPerFrame = this.getFrameTime();
				var clipTime = this.entry.currentFrame * millisPerFrame / 1000;
				var totalClipTime = this.entry.nFrames * millisPerFrame / 1000;
				var labelText =  (1+this.entry.currentFrame) + "/" + this.entry.nFrames;
				//labelText += " -- " + newFrame.fileName;
				this.frameInfoLabel.text(labelText);			// update text
				var progressText = clipTime.toFixed(0) + "/" + totalClipTime.toFixed(0) + "s";
				this.bar.progressbar({value : newFrameIdx / this.entry.nFrames * 100, text : progressText});	// update bar
			}
			
			if (updateOnly != true)
			{
				// caller seeks to a specific frame -> Update play information
				this.updateFrameTime();
			}
			
			if (this.isEOF())
			{
				// last frame -> Pause
				this.pause();
			}
		},

		// set frame time to now
		updateFrameTime :  function()
		{
			this.startFrameIdx = this.entry.currentFrame;
			this.startFrameTime = getCurrentTimeMillis();
		},
		
		// jumps to the next frame
		jumpToNextFrame : function()
		{
			this.setFrame(this.entry.currentFrame+1);		// set frame and frame time
		},
		
		// Sets the current frame, according to time passed
		updateFrame : function()
		{
			if (undefined == this.entry) return;
			
			var timePassed = getCurrentTimeMillis() - this.startFrameTime;
			var framesPassed = Math.floor(timePassed / this.getFrameTime());
			var newFrame = this.startFrameIdx + framesPassed;
			this.setFrame(newFrame, true);					// set frame and don't set frame time
		},
		
		togglePlay : function()
		{
			if (this.isPlaying())
				this.pause();
			else 
				this.play();
		},
		
		// triggered by the "Play" button
		play : function()
		{
			if (!this.isReady()) return;
			
			if (this.isPlaying()) return;
			
			// set last frame time
			this.updateFrameTime();
			
			// check if clip already ended
			if (this.isEOF())
				this.stop();
			
			// start timer
			(function(viewer) {
				viewer.playTimer = setInterval(function() { 
					viewer.updateFrame.call(viewer);
				}, viewer.getPlaybackInterval());
			})(this);
			
			
			// TODO: Use events to let user update the UI
			this.playButton.text("Pause");
		},
		
		// triggered by the "Stop" button
		stop : function()
		{
			if (!this.isReady()) return;
			
			this.pause();								// pause
			
			var frame0;
			if (this.entry)
				frame0 = this.entry.startFrame;
			else
				frame0 = 0;
			this.setFrame(frame0);		// go back to beginning
		},
		
		// triggered by the "Pause" button
		pause : function()
		{
			if (!this.isReady()) return;
			
			if (!this.isPlaying()) return;
			
			clearInterval(this.playTimer);
			this.playTimer = null;
			
			// TODO: Use events to let user update the UI
			this.playButton.text("Play");
		},
		
		// triggered by clicking on the seek bar
		seek : function(ratio)
		{
			this.setFrame(this.getFrameAtRatio(ratio));
		},
		
		
		// ############################################################################################################
		// clip entry management
		
		setFPS : function(entry, fps)
		{
			if (!this.isReady()) return;
			
			entry.fps = fps;
			
			// update text
			for (var i = 0; i < this.clipList[0].options.length; ++i) {
				var option = this.clipList[0].options[i];
				if (option.value == entry.name)
				{
					$(option).text(entry);
				}
			}
			
			// update frame time
			this.updateFrameTime();
		},
		
		// ############################################################################################################
		// misc
		
		toString : function()
		{
			return "Viewer" + this.viewerId;
		},
		
		onFail : function(message)
		{
			this.statusLabel.text(message);
			this.statusLabel.css('background-color', 'red');
		}
	}
	
	return ctor;
})();

function createViewer(elem, cfgDir, cfgFile, viewerId)
{
	var newViewer = $.extend(elem, new _ViewerClassDef(viewerId));
	newViewer.cfgDir = cfgDir;
	newViewer.cfgFile = cfgFile;
	newViewer.setup();
	return newViewer;
}