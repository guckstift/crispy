var a = 90;

function foo()
{
	var x = 90;
	
	function bar()
	{
		print a;
	}
	
	return bar;
}

var bee = foo();
bee();
