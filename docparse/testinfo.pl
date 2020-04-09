#!/usr/bin/perl
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright (c) 2019 Cyril Hrubis <chrubis@suse.cz>
# Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>

use strict;
use warnings;

use JSON;
use LWP::Simple;
use Cwd qw(abs_path);
use File::Basename qw(dirname);

use Data::Dumper;

use constant URL_ALL_TESTS => "index.html";
use constant OUTDIR => dirname(abs_path($0));

sub load_json
{
	my ($fname, $mode) = @_;
	local $/;

	open(my $fh, '<', $fname) or die("Can't open $fname $!");

	return <$fh>;
}

sub log_info
{
	my $msg = shift;
	print STDERR "INFO: $msg\n";
}

sub log_warn
{
	my $msg = shift;
	print STDERR "WARN: $msg\n";
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

	for my $c (@{$config}) {
		my $link = html_a($c->{'file'}, exists($c->{'menu'}) ? $c->{'menu'} : $c->{'subtitle'});
		print $fh "<li>$link</li>\n";
	}

	print $fh <<EOL;
     </ul>
EOL

	# TODO: move letters here
	#my @names = sort keys %{$json->{'tests'}};
	#print $fh get_letters(\@names);

	print $fh <<EOL;
   </div>
EOL

	print $fh <<EOL;
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

sub test_url {
	my ($test) = @_;
	return html_a(URL_ALL_TESTS . "#$test", $test);
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

# TODO: separate getting letters and printing crumbs, as letters are needed also later on
# thus probably content_name() should just pass data to some generic function which print list of tests
sub get_letters
{
	my @names = @{$_[0]};
	my $letter;
	my $prev_letter = "";
	my $content;

	for (@names) {
		$_ = substr($_, 0, 1);
	}
	$content .= "<div>Total $#names tests</div>";
	@names = uniq(@names);

	$content .= "<div id='crumbs'>";
	for my $letter (@names) {
		$content .= html_a("#$letter", $letter) . '&nbsp;';
	}
	$content .= "</div>\n";

	return $content;
}

sub content_name
{
	my $json = shift;
	my @names = sort keys %{$json->{'tests'}};
	my $letter;
	my $prev_letter = "";
	my $content;

	$content .= "<div>";
	$content .= get_letters(\@names);

	for my $name (@names) {
		$letter = substr($name, 0, 1);
		if ($letter ne $prev_letter) {
            $content .= "</ul>" if ($prev_letter);
			$content .= "<div class='letter'><a id='" . $letter . "'></a>" . $letter . "</div><ul>";
		}
		$content .= "<li>" . test_url($name) . "</li>\n";
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

# TODO for title
sub text2title
{
	my $text = shift;
    $text =~ s/[_]/ /g;
	return ($text =~ /need/s ? 'tests' : 'tests by') . " $text";
}

sub tag2title
{
	my $tag = shift;
	return ".$tag";
}

sub detect_git
{
	unless (defined $ENV{'LINUX_GIT'} && $ENV{'LINUX_GIT'}) {
		log_warn("kernel git repository not defined. Define it in \$LINUX_GIT");
		return 0;
	}

	unless (-d $ENV{'LINUX_GIT'}) {
		log_warn("\$LINUX_GIT does not exit ('$ENV{'LINUX_GIT'}')");
		return 0;
	}

	my $ret = 0;
	if (system("which git >/dev/null")) {
		log_warn("git not in \$PATH ('$ENV{'PATH'}')");
		return 0;
	}

	chdir($ENV{'LINUX_GIT'});
	if (!system("git log -1 > /dev/null")) {
		log_info("using '$ENV{'LINUX_GIT'}' as kernel git repository");
		$ret = 1;
	} else {
		log_warn("git failed, git not installed or \$LINUX_GIT is not a git repository? ('$ENV{'LINUX_GIT'}')");
	}
	chdir(OUTDIR);

	return $ret;
}

sub content_all_tests
{
	my $json = shift;
	my $tmp = undef;
	my $content;

	my @names = sort keys %{$json->{'tests'}};
	my $has_kernel_git = detect_git();

	unless ($has_kernel_git) {
		log_info("Parsing git messages from linux git repository skipped due previous error");
	}

	for my $name (@names) {
		$content .= "<hr />\n" if (defined($tmp));

		$content .= "<h2 id=\"$name\">$name<a class=\"anchor\" href=\"#$name\" title=\"Permalink to this test\">¶</a></h2>";

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
		my %commits;

		for my $tag (@{$json->{'tests'}{$name}{tags}}) {
			if (!defined($tmp2)) {
				$content .= "<table><tr><th>Tags</th><th>Info</th></tr>"
			}
			my $k = @$tag[0];
			my $v = @$tag[1];
			my $text = $k;

            if ($has_kernel_git && $k eq "linux-git") {
				$text .= "-$v";
				unless (defined($commits{$v})) {
					chdir($ENV{'LINUX_GIT'});
					$commits{$v} = `git log --pretty=format:'%s' -1 $v`;
					chdir(OUTDIR);
				}
				$v = $commits{$v};
			}
			my $a = html_a(tag_url($k, @$tag[1]), $text);
			$content .= "<tr><td>$a</td><td>$v</td></tr>";
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
		subtitle => "All tests",
		content => \&content_all_tests,
    },
    {
		file => "name.html",
		subtitle => "Tests by name",
		content => \&content_name,
    },
    {
		file => "about.html",
		subtitle => "About",
		content => \&content_about,
		menu => "About testsuite",
    },
];


for my $c (@{$config}) {
	open(my $fh, '>', $c->{'file'}) or die("Can't open $c->{'file'} $!");
	print_html($fh, $json, $c->{'subtitle'}, $c->{'content'}->($json), $config);
}
