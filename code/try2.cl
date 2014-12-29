
print("TRY TEST1...");
try {
    throw new MethodMissingException("Method Missing");
}
catch(MethodMissingException e) {
    println("TRUE");
}
catch(Exception e) {
    println("FALSE");
}
finally {
    println("finally OK");
}

print("TRY TEST2...");
try { 
    throw new MethodMissingException("Method Missing");
}
catch(Exception e) {
    println("TRUE");
}
finally {
    println("finally OK");
}

print("TRY TEST3...");
try { 
    try { 
        throw new MethodMissingException("Method Missing");
    }
    catch(MethodMissingException e) {
        println("TRUE");
    }
}
finally {
    println("finally OK");
}

print("TRY TEST4...");
try { 
    throw new MethodMissingException("Method Missing");
}
catch(MethodMissingException e) {
    try {
        throw new MethodMissingException("Method Missing");
    }
    catch(MethodMissingException e) {
        println("TRUE");
    }
}
finally {
    println("finally OK");
}

print("TRY TEST5...");
try { 
    throw new MethodMissingException("Method Missing");
}
catch(MethodMissingException e) {
    println("OK");
}
finally {
    println("finally OK");
}

print("TRY TEST6...");
print(Clover.outputToString() {
    try {
        for(int i=0; i<1; i++) {
            print("O");
        }
    }
    finally {
        println("K");
    }
});

println("FINISHED");
