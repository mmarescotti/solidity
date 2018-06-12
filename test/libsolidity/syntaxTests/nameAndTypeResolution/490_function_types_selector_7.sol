contract C {
    function h() pure external {
    }
    function f() view external returns (bytes4) {
        function () pure external g = this.h;
        function () pure external i = g;
        return i.selector;
    }
}
// ----
