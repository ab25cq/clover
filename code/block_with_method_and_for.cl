Array<int> array = { 1, 2, 3 };

int k = 123;

array[1] = 4;

print("method block and for block test1...");
for(int i=0; i<array.length(); i++) {
    int l = 777;

    for(int k=0; k<1; k++) {
        int o = 999;

        array.each() { |int a|
            int lll = 1024;

            for(int j=0; j<1; j++) {
                int kkk = 111;
                int zzz = 123;
            }

            int opq = 1024;

            for(int h=0; h<1; h++) {
                int ooo = 777;
                int yyy = 1024;

                array.each() { |int a|
                    int xxx = 512;

                    if(!(k>=0 && k<5 && l == 777 && o == 999 && lll == 1024 && ooo == 777 && xxx == 512 && yyy == 1024)) {
                        println("FALSE");
                        System.exit(2);
                    }
                }
            }
        }
    }
}

println("TRUE");
