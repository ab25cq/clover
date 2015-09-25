
int fd = System.open(p"a.txt", (FileMode.O_CREAT|FileMode.O_TRUNC|FileMode.O_APPEND|FileMode.O_WRONLY).toFileMode());
System.write(fd, B"ABC\n");
System.close(fd);

Bytes data = B"";

int fd2 = System.open(p"a.txt", FileMode.O_RDONLY);

int result = System.read(fd2, data, 5);

print("System.read test1...");
if(result == 4 && data == B"ABC\n") {
    println("TRUE");
}
else {
    println("FALSE");
    System.exit(2);
}
System.close(fd2);

