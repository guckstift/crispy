function foo()
{
	function bar()
	{
	}
	
	return bar;
}

var x = foo();
print x;
