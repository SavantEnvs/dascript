options gen2
def apply(f : block<(x : int) : int>; v : int) : int {
    return invoke(f, v)
}
[export]
def main() {
    let k = 10
    print("{apply($(x : int) => x * k, 5)}\n")
    var total = 0
    for (x in [1, 2, 3, 4]) {
        total += x
    }
    print("{total}\n")
}
