contract C {
	function f() public pure returns (uint, uint, uint, uint) {
		// Can later be replaced by (uint a, uint b,) = f();
		(uint a, uint b,) = f();
		a; b;
	}
}
// ----
// Warning: (131-154): Different number of components on the left hand side (3) than on the right hand side (4).
