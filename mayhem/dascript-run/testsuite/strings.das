options gen2
require strings
[export]
def main() {
    let s = "Hello, daScript"
    print("{length(s)}\n")
    print("{to_upper(s)}\n")
    print("{find(s, "da")}\n")
    print("{slice(s, 7)}\n")
    print("{to_int("1234") + 1}\n")
}
