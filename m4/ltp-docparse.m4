dnl SPDX-License-Identifier: GPL-2.0-or-later
dnl Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>

AC_DEFUN([LTP_CHECK_METADATA_GENERATOR_ASCIIDOCTOR], [
	AC_MSG_NOTICE([checking Asciidoctor])
	AC_PATH_TOOL(asciidoctor, "asciidoctor")
	AC_PATH_TOOL(asciidoctor_pdf, "asciidoctor-pdf")
	metadata_generator_html=$asciidoctor
	metadata_generator_pdf=$asciidoctor_pdf
	echo "ASCIIDOCTOR: metadata_generator_html: '$metadata_generator_html', metadata_generator_pdf: '$metadata_generator_pdf'" # FIXME: debug
])

AC_DEFUN([LTP_CHECK_METADATA_GENERATOR_ASCIIDOC], [
	AC_MSG_NOTICE([checking Asciidoc])
	AC_PATH_TOOL(a2x, "a2x")
	echo "ASCIIDOC: a2x: '$a2x'" # FIXME: debug
	version="`$a2x --version | cut -d ' ' -f2 `"
	echo "a2x version: '$version'" # FIXME: debug
	AX_COMPARE_VERSION([$version], lt, 9, [
		AC_MSG_WARN([a2x unsupported version: $version. Use a2x >= 9])
		a2x=
	])
	metadata_generator_html=$a2x
	metadata_generator_pdf=$a2x
	echo "ASCIIDOC: metadata_generator_html: '$metadata_generator_html', metadata_generator_pdf: '$metadata_generator_pdf'" # FIXME: debug
])

AC_DEFUN([LTP_DOCPARSE], [
# FIXME: debug
echo "metadata=$metadata"
echo "enable_metadata=$enable_metadata"

echo "metadata_html=$metadata_html"
echo "enable_metadata_html=$enable_metadata_html"

echo "metadata_pdf=$metadata_pdf"
echo "enable_metadata_pdf=$enable_metadata_pdf"

echo "metadata_generator=$metadata_generator"
echo "with_metadata_generator=$with_metadata_generator"

with_metadata=no
with_metadata_html=no
with_metadata_pdf=no

if test "x$enable_metadata" = xyes && test "x$enable_metadata_html" = xyes -o "x$enable_metadata_pdf" = xyes; then
	AX_PROG_PERL_MODULES(Cwd File::Basename JSON LWP::Simple)
fi
echo "ax_perl_modules_failed: '$ax_perl_modules_failed'" # FIXME: debug

if test "x$ax_perl_modules_failed" = x0; then
	echo "MAYBE ENABLED =====" # FIXME: debug
	if test "x$with_metadata_generator" = xasciidoctor -o "x$with_metadata_generator" = xdetect; then
		LTP_CHECK_METADATA_GENERATOR_ASCIIDOCTOR
	elif test "x$with_metadata_generator" = xasciidoc; then
		LTP_CHECK_METADATA_GENERATOR_ASCIIDOC
	else
		AC_MSG_ERROR([invalid metadata generator '$with_metadata_generator', use --with-metadata-generator=asciidoc|asciidoctor])
	fi
	# FIXME: debug
	echo "0. metadata_generator_html: '$metadata_generator_html'"
	echo "0. metadata_generator_pdf: '$metadata_generator_pdf'"
	# FIXME: debug

	# autodetection: check also Asciidoc
	if test "x$with_metadata_generator" = xdetect; then
		with_metadata_generator='asciidoctor'
		echo "1. asciidoctor: metadata_generator_html: '$metadata_generator_html', enable_metadata_html: '$enable_metadata_html'" # FIXME: debug
		echo "1. asciidoctor: metadata_generator_pdf: '$metadata_generator_pdf', enable_metadata_pdf: '$enable_metadata_pdf'" # FIXME: debug
		# problems with Asciidoctor: (html enabled && not found) || (pdf enabled && not found) => try Asciidoc
		if test "x$enable_metadata_html" = xyes -a "x$metadata_generator_html" = x ||
			test "x$enable_metadata_pdf" = xyes -a "x$metadata_generator_pdf" = x; then
			backup_html="$metadata_generator_html"
			backup_pdf="$metadata_generator_pdf"
			with_metadata_generator='asciidoc'
			LTP_CHECK_METADATA_GENERATOR_ASCIIDOC
			echo "2. asciidoc: metadata_generator_html: '$metadata_generator_html', enable_metadata_html: '$enable_metadata_html'" # FIXME: debug
			echo "2. asciidoc: metadata_generator_pdf: '$metadata_generator_pdf', enable_metadata_pdf: '$enable_metadata_pdf'" # FIXME: debug
			# prefer Asciidoctor if it's not worse than Asciidoc
			# (html not enabled || asciidoctor html found || asciidoc html not found) && (pdf ...)
			if test "x$enable_metadata_html" != xyes -o "x$backup_html" != x -o "x$metadata_generator_html" = x &&
				test "x$enable_metadata_pdf" != xyes -o "x$backup_pdf" != x -o "x$metadata_generator_pdf" = x; then
				with_metadata_generator='asciidoctor'
				echo "CHOOSE asciidoctor ($with_metadata_generator), was better than asciidoc" # FIXME: debug
				metadata_generator_html="$backup_html"
				metadata_generator_pdf="$backup_pdf"
			else
				echo "KEEP asciidoc ($with_metadata_generator), was better than asciidoctor" # FIXME: debug
			fi
		else
			echo "KEEP asciidoctor ($with_metadata_generator), didn't try asciidoc" # FIXME: debug
		fi
		if test "x$metadata_generator_html" != x -o "x$metadata_generator_pdf" != x; then
			AC_MSG_NOTICE(choosing $with_metadata_generator for metadata generation)
		else
			echo "NO GENERATOR DETECTED" # FIXME: debug
		fi
	fi

	if test "x$enable_metadata_html" = xno; then
		AC_MSG_NOTICE(HTML metadata generation disabled)
	elif test "x$metadata_generator_html" != x; then
		with_metadata_html=yes
	fi

	if test "x$enable_metadata_pdf" = xno; then
		AC_MSG_NOTICE(PDF metadata generation disabled)
	elif test "x$metadata_generator_pdf" != x; then
		with_metadata_pdf=yes
	fi
fi

# FIXME: debug
echo "with_metadata_html: '$with_metadata_html'"
echo "with_metadata_pdf: '$with_metadata_pdf'"
echo "last: metadata_generator_html: '$metadata_generator_html'"
echo "last: metadata_generator_pdf: '$metadata_generator_pdf'"
# FIXME: debug

reason="metadata generation skipped due missing generator"
hint="specify correct generator with --with-metadata-generator=asciidoc|asciidoctor or use --disable-metadata|--disable-metadata-html|--disable-metadata-pdf"

if test -z "$ax_perl_modules_failed"; then
	echo "DISABLED by user :( =====" # FIXME: debug
	AC_MSG_NOTICE(metadata generation disabled)
elif test "x$ax_perl_modules_failed" = x1; then
	AC_MSG_WARN(metadata generation skipped due missing required Perl modules)
elif test "x$with_metadata_html" = xno -a "x$with_metadata_pdf" = xno; then
	AC_MSG_WARN([$reason, $hint])
	echo "DISABLED missing dependencies :( =====" # FIXME: debug
else
	with_metadata=yes
	AC_SUBST(METADATA_GENERATOR, $with_metadata_generator)
	echo "ENABLED: $with_metadata_generator :) =====" # FIXME: debug
	if test "x$with_metadata_html" = xno -a "x$enable_metadata_html" = xyes; then
		AC_MSG_WARN([HTML $reason, $hint])
	fi
	if test "x$with_metadata_pdf" = xno -a "x$enable_metadata_pdf" = xyes; then
		AC_MSG_WARN([PDF $reason, $hint])
	fi
fi

AC_SUBST(WITH_METADATA, $with_metadata)
AC_SUBST(WITH_METADATA_HTML, $with_metadata_html)
AC_SUBST(WITH_METADATA_PDF, $with_metadata_pdf)
])
