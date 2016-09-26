$.extend({
	_log: function(message){
		window.console && window.console.log(message);
	},
	isEmpty: function(strObj){
		if((typeof strObj == 'undefined') || null == strObj || '' == $.trim(strObj) ){
			return true;
		}else{
			return false;
		}
	}

});