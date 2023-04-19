function foo()
{
	print "foo";
}

function foo2()
{
	print "foo2";
}

function foo3()
{
	print "foo3";
	return foo2();
}

#print foo();
#print foo() + foo2();
print foo() + foo2() - foo3();
#var x = 2;
#print 1 + x - 3;
#print 1 + x - foo();

var x = foo() + foo2() - foo3();
x = foo() + foo2() - foo3();
