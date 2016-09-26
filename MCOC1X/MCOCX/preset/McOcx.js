$.extend({
	_invoke: function(obj, method){
		var params = new Array();
		for(var i = 2; i < arguments.length; i++) {
			if (typeof(arguments[i]) == "number") {
				params.push(arguments[i]);
			} else {
				params.push("'" + arguments[i] + "'");
			}
		}

		if (method == "SetLayOut") {
			$._invoke(obj, "ResetLayOut");
		}

		var excludes = ["StartRealPlay", "StartRealPlaybyServiceUrl", "StartPlayBack", "StartPlayBackEx", "StartPlayByOneMergeScreen"];
		if($.inArray(method, excludes) > -1) {
		
		}

		var args = params.join(", ");
			
		var specials = ["StartRealPlay", "StartRealPlaybyServiceUrl", "StartPlayBack", "StartPlayBackEx", "StartDownloadEx"];
		if($.inArray(method, specials) > -1) {
			var sid = $._invoke(obj, "GetPlaySession");
			args = sid + ", " + args;
		}
		$._log("<" + $(obj).attr("id") + "> : invoke method ( " + method + " ) with arguments ( " + args + " ) ...");
		
		return eval("obj[method](" + args + ")");
	},
	StopPalyer: function(obj, wid, callBack){
		var ret = $._invoke(obj, "StopPlayByWinID", wid);
		if(ret === 0){
			if(typeof callBack == 'function') callBack();
		}
	}

});