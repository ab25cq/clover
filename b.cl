Array<String> result = new Array<String>();

String env_path = System.getenv("PATH");
String path = "";

for(int offset = 0; offset<env_path.length(); offset++) {
    if(env_path[offset] == ':') {
        if(File.isDirectory(path)) {
            new Directory(path).each() {|String file_name|
                result.append(file_name);
            }
        }

        path = "";
    }
    else {
        path += env_path[offset].toCharacter();
    }
}

if(File.isDirectory(path)) {
    new Directory(path).each() {|String file_name|
        result.append(file_name);
    }
}

result.toString().println();

