contract C {
    function f() public { }
    function g() public {
        // FIXME: does this test still make sense? (void -> "uint")
        (uint x,) = (f(), f());
    }
}
// ----
// Warning: (156-159): Tuple component cannot be empty.
// Warning: (161-164): Tuple component cannot be empty.
// TypeError: (143-165): Type tuple() is not implicitly convertible to expected type uint256.
