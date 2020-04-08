// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2019 Cyril Hrubis <chrubis@suse.cz>
 * Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>
*/

$.urlParam = function(name){
    var results = new RegExp('[\?&]' + name + '=([^&#]*)').exec(window.location.href);
    return results ? results[1] : 0;
}

function tag_url(metadata, tag, value) {
    if (tag == "CVE")
        return "https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-" + value;
    if (tag == "linux-git")
        return "https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=" + value;
    if (tag == "fname")
        return metadata.scm_url_base + value;
}

function get_linux_git_commit(hash) {
    var request = new XMLHttpRequest();
    request.open("GET", "https://api.github.com/repos/torvalds/linux/commits/" + hash, false);
    request.overrideMimeType("application/json");
    request.send(null);
    return JSON.parse(request.responseText);
}

function test_info_link(test, link) {
    return "<a href=?test=" + test + ">" + link + "</a>"
}

function tests_by_attr_vals(metadata, attr) {
    var html = "";
    var by_keys = tests_by_keys_tags(metadata)[0];
    var attrs = {};
    var by_attrs = by_keys[attr];

    for (var i in by_attrs) {
        var test_name = by_attrs[i][0];
        var test_attr_val = by_attrs[i][1];

        if (Array.isArray(test_attr_val)) {
            for (var j in test_attr_val) {
                var val = test_attr_val[j];
                if (!attrs[val])
                    attrs[val] = [];
                attrs[val].push(test_name);
            }
        } else {
            if (!attrs[test_attr_val])
                attrs[test_attr_val] = [];
            attrs[test_attr_val].push(test_name);
        }
    }

    for (var attr in attrs) {
        if (Object.keys(attrs).length > 1)
            html += "<h3>" + attr + "</h3>";
        html += "<ul>";
        for (var i in attrs[attr]) {
            var test = attrs[attr][i];
            html += "<li>" + test_info_link(test, test) + "</li>";
        }
        html += "</ul>";
    }

    return html;
}

function tests_by_keys_tags(metadata) {
    var tests = metadata["tests"];
    var by_tags = {};
    var by_keys = {};

    for (var key in tests) {
        var tdata = tests[key];
        var tags = tdata["tags"];

        for (var attr in tdata) {
            if (!by_keys[attr])
                by_keys[attr] = [];
            by_keys[attr].push([key, tdata[attr]]);
        }

        if (!tags)
            continue;

        for (var i in tags) {
            var tag = tags[i][0];
            if (!by_tags[tag])
                by_tags[tag] = [];
            by_tags[tag].push([key, tags[i][1]]);
        }
    }

    return [by_keys, by_tags];
}

$(function() {
    $("<h1>" + metadata.testsuite_short + " test catalog</h1>").appendTo("#header");
    $("<div>" + metadata.testsuite + " " + metadata.version + "</div>").appendTo("#footer");

    var sort = $.urlParam('sort');
    var test = $.urlParam('test');
    var html = "";
    var h2url, h2text;

    if (test) {
        var data = metadata.tests[test];
        h2text = test;
        h2url = tag_url(metadata, "fname", data.fname);
        $.each(data.doc, function(i, item) {
            html += "<div>" + item + "</div>";
        });

        if (data.timeout) {
            if (data.timeout == -1)
                html += "<div>Test timeout is disabled</div>";
            else
                html += "<div>Test timeout is " + data.timeout + " seconds</div>";
        } else {
            html += "<div>Test timeout defaults to " + metadata.timeout + " seconds</div>";
        }

        // keys
        html += "<table><tr><th>Key</th><th>Value</th></tr>";
        $.each(data, function(key, item) {
            if (key == "tags" || key == "fname" || key == "doc")
                return;
            html += "<tr>";
            html += "<td>" + key + "</td>";
            html += "<td>" + data[key] + "</td>";
            html += "</tr>";
        });
        html += "</table>";

        // tags
        if (data.tags) {
            html += "<table><tr><th>Tags</th><th>Info</th></tr>";
            $.each(data.tags, function(i, item) {
                html += "<tr>";
                html += "<td><a href=\"" + tag_url(metadata, item[0], item[1]) + "\">" + item[0] + "-" + item[1] + "</a></td>";
                if (item[0] == "linux-git") {
                    commit = get_linux_git_commit(item[1]);
                    message = commit["commit"]["message"].split("\n");
                    html += "<td>" + message[0] + "</td>";
                }
                html += "</tr>";
            });
            html += "</table>";
        }
    } else if (sort) { // sort
        var data = metadata["tests"];
        if (sort == "names") {
            h2text = "Tests by name";
            html += "<div>Total " + Object.keys(data).length + " tests</div>";

            var keys = $.map(data, function(key, item) {return item}).sort();

            var uniq = $.map(keys, function(item) {return item.charAt(0)});
            uniq = [... new Set(uniq)];

            html += "<div id='crumbs'>";
            $.each(uniq, function(key, item) {
                var first = item.charAt(0);
                html += "<a href='#" + first + "'>" + first + "</a>" + "&nbsp;";
            });
            html += "</div>";

            var prev_first;
            $.each(keys, function(key, item) {
                var first = item.charAt(0);
                if (first != prev_first) {
                    if (prev_first)
                        html += "</ul>";
                    html += "<div class='letter'><a id='" + first + "'></a>" + first + "</div><ul>";
                }

                html += "<li>" + test_info_link(item, item) + "</li>";
                prev_first = first;
            });
            html += "</ul>";
        }

        if (sort == "tags") {
            var by_tags_keys = tests_by_keys_tags(metadata);
            var by_keys = by_tags_keys[0];
            h2text = "Tests by tags";
            $.each(by_tags_keys[1], function(key, item) {
                html += "<h3>" + key + "</h3>";
                html += "<ul>";
                $.each(by_tags_keys[1][key], function(key2, item2) {
                    if (key == "CVE")
                        item2[1] = "CVE-" + item2[1];
                    html += "<li><a href='" + "?test=" + item2[0] + "'>" + item2[1] + "</a></li>";
                    html += "<li>" + test_info_link(item2[0], item2[1]) + "</li>";
                });
                html += "</ul>";
            });
        }

        if (sort == "drivers") {
            h2text = "Tests by drivers";
            html += tests_by_attr_vals(metadata, "needs_drivers");
        }

        if (sort == "kver") {
            h2text = "Tests by minimal kernel version";
            html += tests_by_attr_vals(metadata, "min_kver");
        }

        if (sort == "fs") {
            h2text = "Tests by filesystem type";
            html += tests_by_attr_vals(metadata, "dev_fs_type");
        }

        if (sort == "root") {
            h2text = "Tests that needs root";
            html += tests_by_attr_vals(metadata, "needs_root");
        }

        if (sort == "device") {
            h2text = "Tests that needs device";
            html += tests_by_attr_vals(metadata, "needs_device");
        }
    } else { // main page
        h2text = metadata.testsuite;
        if (metadata.url)
            h2url = metadata.url;

        html += "<div id='version'>";
        if (metadata.version)
            html += "<div><b>Version:</b> " + metadata.version + "</div>";

        if (metadata.timeout)
            html += "<div><b>Default timeout:</b> " + metadata["timeout"] + " seconds</div>";
        html += "</div>";
    }

    if (h2url)
        $("<a>").attr("href", h2url).text(h2text).appendTo("h2");
    else
        $("h2").text(h2text)

	document.title = h2text + " (" + metadata.testsuite + ")";

    $(html).appendTo("#content");
});
