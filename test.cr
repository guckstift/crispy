function foo()
{
	var x = 0;
	
	function enc(step)
	{
		var y = 10;
		
		function bar()
		{
			print x, y;
			x = x + step;
			y = y - step;
		}
		
		return bar;
	}
	
	return enc(2);
}

var closure = foo();

closure();
closure();
print [];
closure();

closure = null;
foo = null;
print [];

