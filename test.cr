function foo()
{
	print [1,2][4];
	var x = "He";
	print x[[]];
}

function bar()
{
	if true {
		foo();
	}
}

function baz()
{
	print "mkay";
	bar();
}

if true {
	baz();
}
