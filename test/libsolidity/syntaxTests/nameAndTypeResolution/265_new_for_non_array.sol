contract C {
    function f(uint size) public {
        uint[] storage x = new uint(7);
    }
}
// ----
// TypeError: (75-83): Contract or array type expected.
