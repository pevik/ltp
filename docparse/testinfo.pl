#!/usr/bin/perl
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2019 Cyril Hrubis <chrubis@suse.cz>
# Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>

use strict;
use warnings;

use JSON;
use Data::Dumper;

use constant URL_ALL_TESTS => "index.html";

sub load_json
{
	my ($fname, $mode) = @_;
	local $/;

	open(my $fh, '<', $fname) or die("Can't open $fname $!");

	return <$fh>;
}

sub print_html
{
	my ($fh, $json, $subtitle, $content, $config) = @_;

	my $h1 = $json->{'testsuite_short'} . " test catalog";
	$h1 .= " &ndash; $subtitle" if (defined $subtitle);

	print $fh <<EOL;
<!doctype html>
<html>
 <head>
  <script type="text/javascript" src="jquery-3.4.1.min.js"></script>
  <link href="metadata.css" media="all" rel="stylesheet" type="text/css" />
  <meta charset="utf-8" />
  <meta name="description" content="Test catalog" />
  <meta name="keywords" content="test,testsuite" />
  <title>Test Catalog &ndash; $subtitle</title>
 </head>

 <body>
  <div id="body">
   <div id="header"><h1>$h1</h1></div>

   <div id="content">
      <div>
      $content
      </div>
   </div>

   <div id="navbar">
     <div>Test listing</div>
     <ul>
EOL

	for my $data (@{$config}) {
		print $fh "<li><a href=\"$data->{'file'}\">" .
			ucfirst(exists($data->{'menu'}) ? $data->{'menu'} : $data->{'subtitle'}) .
			"</a></li>\n";
	}

	print $fh <<EOL;
     </ul>
   </div>

   <div id="footer">$json->{'testsuite'} $json->{'version'}</div>

 </body>
</html>
EOL
}


sub tag_url {
	my ($tag, $value, $scm_url_base) = @_;

    if ($tag eq "CVE") {
        return "https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-" . $value;
	}
    if ($tag eq "linux-git") {
        return "https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=" . $value;
	}
    if ($tag eq "fname") {
        return $scm_url_base . $value;
	}
}

sub html_a
{
	my ($url, $text) = @_;
	return "<a href=\"$url\">$text</a>";
}

sub content_about
{
	my $json = shift;
	my $content;

	my $h2 = ($json->{'url'} ? html_a($json->{'url'}, $json->{'testsuite'}) :
		$json->{'testsuite'});
	$content .= "<h2>$h2</h2>";

	$content .= "<div id='version'>";
	if ($json->{'version'}) {
		$content .="<div><b>Version:</b> $json->{'version'}</div>";
	}
	if ($json->{'timeout'}) {
		$content .= "<div><b>Default timeout:</b> $json->{'timeout'} seconds</div>";
	}
	$content .= "</div>";

	return $content;
}

sub uniq {
	my %seen;
	grep !$seen{$_}++, @_;
}

sub test_info_link {
	my ($test) = @_;
	return "<a href=\"" . URL_ALL_TESTS . "#$test\">$test</a>";
}

sub content_name
{
	my $json = shift;
	my @names = sort keys %{$json->{'tests'}};
	my @letters = @names;
	my $letter;
	my $prev_letter = "";
	my $content;

	for (@letters) {
		$_ = substr($_, 0, 1);
	}
	@letters = uniq(@letters);

	$content .= "<div>Total $#names tests";

	$content .= "<div id='crumbs'>";
	for my $letter (@letters) {
		$content .= "<a href='#$letter'>$letter</a>&nbsp;";
	}
	$content .= "</div>\n";

	for my $name (@names) {
		$letter = substr($name, 0, 1);
		if ($letter ne $prev_letter) {
            $content .= "</ul>" if ($prev_letter);
			$content .= "<div class='letter'><a id='" . $letter . "'></a>" . $letter . "</div><ul>";
		}
		$content .= "<li>" . test_info_link($name) . "</li>\n";
		$prev_letter = $letter;
	}

	$content .= "</ul>";
	$content .= "</div>\n";

	return $content;
}

sub get_data
{
	my $json = shift;
	my %data;
	while (my ($k, $v) = each %{$json->{'tests'}}) {
		for my $j (keys %{$v}) {
			next if ($j eq 'fname' || $j eq 'doc');
			$data{$j} = () unless (defined($data{$j}));
			push @{$data{$j}}, $k;
		}
	}
	return \%data;
}

sub text2url
{
	my $text = shift;
    $text =~ s/[^a-zA-Z0-9]/-/g;
	return "$text.html"
}

sub text2title
{
	my $text = shift;
    $text =~ s/[_]/ /g;
	return ($text =~ /need/s ? 'tests' : 'tests by') . " $text";
}

# TODO: use $data
sub content_tags
{
	my $json = shift;
	my $content;

	$content .= "<div>TODO: tags";
	$content .= "</div>";
	return 0; # FIXME: debug

	# TODO: last
	while (my ($k, $v) = each %{$json->{'tests'}}) {
		if (defined %$v{tags} && ref(%$v{tags}) eq 'ARRAY' && $#{%$v{tags}} ge 0) {
			# TODO: get array size
			# FIXME: debug
			print "$k has tags: " . ( (%$v{tags})). "\n"; # FIXME: debug
			print "k:";
			print Dumper($k);
			print "size: " . $#{%$v{tags}};
			print ", v:";
			print Dumper($v);
			print "v ($k) tags:";
			print Dumper(%{$v}{'tags'});

			for my $i (@{%{$v}{'tags'}}) {
				print "k: $k ($$v{'fname'}): i[0]: $$i[0], i[1]: $$i[1]\n";
			}
			print "-----\n";
			# FIXME: debug
		}
	}

	return $content;
}

sub content_kver
{
	my $json = shift;
	my $content;

	$content .= "<div>TODO: kver";
	$content .= "</div>";

	return $content;
}

sub content_drivers
{
	my $json = shift;
	my $content;

	$content .= "<div>TODO: drivers";
	$content .= "</div>";

	return $content;
}

sub content_filesystem
{
	my $json = shift;
	my $content;

	$content .= "<div>TODO: filesystem";
	$content .= "</div>";

	return $content;
}

sub content_neeeds_root
{
	my $json = shift;
	my $content;

	$content .= "<div>TODO: needs root";
	$content .= "</div>";

	return $content;
}

sub content_neeeds_device
{
	my $json = shift;
	my $content;

	$content .= "<div>TODO: needs device";
	$content .= "</div>";

	return $content;
}

sub content_all_tests
{
	my $json = shift;
	my $tmp = undef;
	my $content;

	my @names = sort keys %{$json->{'tests'}};
	for my $name (@names) {
		$content .= "<hr />\n" if (defined($tmp));

		$content .= "<h2>$name<a class=\"anchor\" href=\"#$name\" title=\"Permalink to this test\">¶</a></h2>";

		if (defined($json->{'scm_url_base'}) &&
			defined($json->{'tests'}{$name}{fname})) {
			$content .= html_a(tag_url("fname", $json->{'tests'}{$name}{fname},
					$json->{'scm_url_base'}), "source");
		}

		if (defined $json->{'tests'}{$name}{doc}) {
			$content .= "<div>";
			for (@{$json->{'tests'}{$name}{doc}}) {
				$content .= "<div>$_</div>";
			}
			$content .= "</div>";
		}

		if ($json->{'tests'}{$name}{timeout}) {
			if ($json->{'tests'}{$name}{timeout} eq -1) {
				$content .= "<div>Test timeout is disabled</div>";
			} else {
				$content .= "<div>Test timeout is $json->{'tests'}{$name}{timeout} seconds</div>";
			}
		} else {
			$content .= "<div>Test timeout defaults to $json->{'timeout'} seconds</div>";
		}

		my $tmp2 = undef;
		# FIXME: use get_data()
		while (my ($k, $v) = each %{$json->{'tests'}{$name}}) {
			next if ($k eq "tags" || $k eq "fname" || $k eq "doc");
			if (!defined($tmp2)) {
				$content .= "<table><tr><th>Key</th><th>Value</th></tr>"
			}

			$content .= "<tr><td>$k</td><td>";
			if (ref($v) eq 'ARRAY') {
				$content .= join(', ', @$v),
			} else {
				$content .= $v;
			}
			$content .= "</td></tr>";

			$tmp2 = 1;
		}
        $content .= "</table>";

		$tmp2 = undef;
		for my $tag (@{$json->{'tests'}{$name}{tags}}) {
			if (!defined($tmp2)) {
				$content .= "<table><tr><th>Tags</th><th>Info</th></tr>"
			}
			my $k = html_a(tag_url(@$tag[0], @$tag[1]), @$tag[0]);
			my $v = @$tag[1];
            if (@$tag[0] eq "linux-git") {
=cut
TODO: get linux kernel commit message
                    commit = get_linux_git_commit(item[1]);
                    message = commit["commit"]["message"].split("\n");
                    html += "<td>" + message[0] + "</td>";
function get_linux_git_commit(hash) {
    var request = new XMLHttpRequest();
    request.open("GET", "https://api.github.com/repos/torvalds/linux/commits/" + hash, false);
    request.overrideMimeType("application/json");
    request.send(null);
    return JSON.parse(request.responseText);
}
=cut
			}
			$content .= "<tr><td>$k</td><td>$v</td></tr>";
			$tmp2 = 1;
		}
        $content .= "</table>";

		$tmp = 1;
	}

	return $content;
}


my $json = decode_json(load_json($ARGV[0]));

my $config = [
    {
		file => URL_ALL_TESTS,
		subtitle => "all tests",
		content => \&content_all_tests,
    },
    {
		file => "name.html",
		subtitle => "tests by name",
		content => \&content_name,
    },
    {
		file => "about.html",
		subtitle => "about",
		content => \&content_about,
		menu => "About testsuite",
    },
];
=cut
    {
		file => "tags.html",
		subtitle => "tests by tags",
		content => \&content_tags,
    },
    {
		file => "kver.html",
		subtitle => "tests by min kver",
		content => \&content_kver,
    },
    {
		file => "drivers.html",
		subtitle => "tests by drivers",
		content => \&content_drivers,
    },
    {
		file => "filesystem.html",
		subtitle => "tests by filesystem",
		content => \&content_filesystem,
    },
    {
		file => "needs-root.html",
		subtitle => "tests needs root",
		content => \&content_neeeds_root,
    },
    {
		file => "needs-device.html",
		subtitle => "tests needs device",
		content => \&content_neeeds_device,
    },
=cut

my $data = get_data($json);

for my $k (keys %$data) {
	my %c = (file => text2url($k), subtitle => text2title($k), content => \&content_tags);
	push @{$config}, \%c;
}

for my $c (@{$config}) {
	open(my $fh, '>', $c->{'file'}) or die("Can't open $c->{'file'} $!");
	print_html($fh, $json, $c->{'subtitle'}, $c->{'content'}->($json), $config);
}
