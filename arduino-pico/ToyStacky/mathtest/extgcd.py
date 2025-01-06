def extgcd (this, other):
	s = 0
	olds = 1
	t = 1
	oldt = 0
	r = int (other)
	oldr = int(this)

	while r != 0:
		q = oldr // r

		newr = oldr - q*r
		oldr = r
		r = newr

		news = olds - q*s
		olds = s
		s = news

		newt = oldt - q*t
		oldt = t
		t = newt

	gcd = a * olds + b * oldt
	print(f"x = {olds}, y = {oldt}")
	if gcd == 1:
		if (olds < 0):
			olds += b
	else:
		olds = 0
	return gcd, olds

a = 4029

first = True
while True:
	try:
		if first:
			a, b = int(input()), int(input())
		else:
			a, b = b, int(input())
		first = False
	except:
		exit()

	print(f"a = {a}, b = {b}")
	gcd, x = extgcd(a, b)
	print(f"gcd = {gcd}, modular inv = {x}")

