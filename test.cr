var x = 90;
print x + 0xff;
print x;
print "Hello";

function foo(x)
{
	if x {
		print "foo is okay";
	}
	else {
		print "foo is bad";
	}
}

foo(true);

foo(false);
