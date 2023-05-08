function foo1()
{
	function bar()
	{
		print "Hello";
	}
	
	return bar;
}

function foo2(x, y, z)
{
}

var bar = foo1();
foo2(4,8,15);
bar();
