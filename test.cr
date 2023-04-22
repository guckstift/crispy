var y;

function foo()
{
	var x = [0];
	y = x;
	x[0] = x;
}

foo();
