Array<String> result = new Array<String>();

String env_path = System.getenv("PATH");
Path path = p"";

for(int offset = 0; offset<env_path.length(); offset++) {
    if(env_path[offset] == ':') {
        if(path.existance() && path.to_stat().S_ISDIR()) {
            path.entries().each() {|Path program|
                if(program.existance() && program.to_stat().S_IXUGO()) {
                    result.add(program.toString());
                }
            }
        }

        path = p"";
    }
    else {
        path += env_path[offset].toString().toPath();
    }
}

if(path.existance() && path.to_stat().S_ISDIR()) {
    path.entries().each() { |Path program|
        if(program.existance() && program.to_stat().S_IXUGO()) {
            result.add(program.toString());
        }
    }
}

