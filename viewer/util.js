function getCallerInfo(){
    try { throw Error('') } catch(err) { 				
		var callerLine = err.stack.split("\n")[4];		// get line of caller
		var index = callerLine.indexOf("at ");
		return callerLine.slice(index+3, callerLine.length); 
	}
}

function assert(stmt, msg)
{
	if (window.disableAssert) return;
	
	if (!stmt)
	{
		var info = "";
		if (msg)
		{
			info += msg + " -- ";
		}
		info += "ASSERTION FAILED at " + getCallerInfo();
		throw info;
	}
}

// see: http://stackoverflow.com/questions/5533192/how-to-get-object-length-in-jquery
function getObjectSize(obj)
{
	var size = 0;
	for (var i in obj) {
		++size;
	}
	return size;
}

// Checks whether the given type indicates that the object has been declared and assigned a value.
function isDefinedType(objType)
{
	return objType != "undefined";
}

// Checks whether the given object has been assigned a value and is not null nor false.
function isSet(obj)
{
	return obj != null && obj != false;
}

function hasProperty(obj, key)
{
	return getObjectSize(obj.key) == 0;
}

// Returns the first property of the given object
function getFirstProperty(obj)
{
	for (var prop in obj)
		return obj[prop];
}

// echo object to console
function logObject(msg, obj)
{
	console.log(msg + ": " + JSON.stringify(obj, null, 4) );
}

// Returns the current system time in milliseconds for global synchronization and timing events
function getCurrentTimeMillis()
{
	return new Date().getTime();
}


// add some utilities to jQuery
if (window.jQuery)
{
	$( document ).ready(function() {
		// add centering functionality to jQuery components
		// see: http://stackoverflow.com/questions/950087/how-to-include-a-javascript-file-in-another-javascript-file
		jQuery.fn.center = function (relativeParent) {
			if (undefined == relativeParent) relativeParent = $(window);
			var elem = $(this);
			
			var parentOffset = relativeParent.offset();
			var leftOffset = Math.max(0, ((relativeParent.outerWidth() - elem.outerWidth()) / 2) + relativeParent.scrollLeft());
			var topOffset = Math.max(0, ((relativeParent.outerHeight() - elem.outerHeight()) / 2) + relativeParent.scrollTop())
			if (undefined != parentOffset)
			{
				leftOffset += parentOffset.left;
				topOffset += parentOffset.top;
			}
			elem.offset({left : leftOffset, top : topOffset});
			return this;
		};
		
		jQuery.fn.centerWidth = function (relativeParent) {
			if (undefined == relativeParent) relativeParent = $(window);
			var elem = $(this);
			
			
			var parentOffset = relativeParent.offset();
			var leftOffset = Math.max(0, ((relativeParent.outerWidth() - elem.outerWidth()) / 2) + relativeParent.scrollLeft());
			if (undefined != parentOffset)
			{
				leftOffset += parentOffset.left;
			}
			elem.offset({left : leftOffset, top : elem.offset().top});
			return this;
		};
		
		// Add text width measurement tool to jQuery components
		// see: http://stackoverflow.com/questions/1582534/calculating-text-width-with-jquery
		$.fn.textWidth = function(){
		  var html_org = $(this).html();
		  var html_calc = '<span>' + html_org + '</span>';
		  $(this).html(html_calc);
		  var width = $(this).find('span:first').width();
		  $(this).html(html_org);
		  return width;
		};
	});
}