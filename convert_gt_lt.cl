StringBuffer result = new StringBuffer("");

p"./index.html".readOut().toString().lines().each() {|String line|
    result += line.sub(/<|>/g, { "<" => "&lt;", ">" => "&gt;" });
}
result.toString().println();

