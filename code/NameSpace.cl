
namespace Test;

include "Fundamental.cl";

inherit class Array {
    void print () {
        ::Clover.print("Using default namespace to Clover.print");
        Clover.print("Using Test namespace to Clover.print");
    }
}
