options gen2
struct Point {
    x : int
    y : int
}
[export]
def main() {
    var a : array<int>
    for (i in range(5)) {
        push(a, i * i)
    }
    print("{length(a)} {a[4]}\n")
    var t : table<string; int>
    t |> insert("one", 1)
    t |> insert("two", 2)
    print("{length(t)} {t?["two"] ?? -1} {t?["three"] ?? -1}\n")
    let p = Point(x = 3, y = 4)
    print("{p.x * p.x + p.y * p.y}\n")
    delete a
    delete t
}
