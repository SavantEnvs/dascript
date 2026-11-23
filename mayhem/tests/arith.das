options gen2
def fib(n : int) : int {
    return n < 2 ? n : fib(n - 1) + fib(n - 2)
}
[export]
def main() {
    print("{fib(20)}\n")
    print("{7 / 2} {7 % 3} {1 << 10} {0xff & 0x0f}\n")
    var s = 0
    for (i in range(101)) {
        s += i
    }
    print("{s}\n")
}
