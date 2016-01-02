Array<GenericsParametor> parametors = ClassGenericsInfoTest$2->toClass().genericsParametorTypes();

print("ClassGenericsInfoTest test...");
Clover.assert(parametors[0].extendsType() == int && parametors[1].implementedInterfaces() == {ICloneable, IInspectable});
println("TRUE");

