
namespace Test;

include "Fundamental.cl";

inherit class Array {
    void println () {
        ::Clover.println("Using default namespace to Clover.println");
        Clover.println("Using Test namespace to Clover.println");
    }
}
