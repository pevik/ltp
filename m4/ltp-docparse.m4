dnl SPDX-License-Identifier: GPL-2.0-or-later
dnl Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>

AC_DEFUN([LTP_CHECK_METADATA_GENERATOR_ASCIIDOCTOR], [
	AC_MSG_NOTICE([checking Asciidoctor])
	AC_PATH_TOOL(metadata_generator_html, "asciidoctor")
	AC_PATH_TOOL(metadata_generator_pdf, "asciidoctor-pdf")
])

AC_DEFUN([LTP_CHECK_METADATA_GENERATOR_ASCIIDOC], [
	AC_MSG_NOTICE([checking Asciidoc])
	AC_PATH_TOOL(metadata_generator_html, "a2x")
	metadata_generator_pdf=$metadata_generator_html
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

if test "x$enable_metadata" = xyes; then
	AX_PROG_PERL_MODULES(Cwd File::Basename JSON LWP::Simple, ,
	AC_MSG_WARN(Missing required Perl modules)
	)
fi
echo "ax_perl_modules_failed: '$ax_perl_modules_failed'" # FIXME: debug

if test "x$ax_perl_modules_failed" = xyes && test "x$enable_metadata_html" = xyes -o "x$enable_metadata_pdf" = xyes; then
	echo "ENABLED" # FIXME: debug
	if test "x$with_metadata_generator" = xasciidoctor -o "x$with_metadata_generator" = xdetect; then
		LTP_CHECK_METADATA_GENERATOR_ASCIIDOCTOR
	elif test "x$with_metadata_generator" = xasciidoc; then
		LTP_CHECK_METADATA_GENERATOR_ASCIIDOC
	else
		AC_MSG_ERROR([invalid metadata generator '$with_metadata_generator', use --with-metadata-generator=asciidoc|asciidoctor])
	fi
	# FIXME: debug
	echo "metadata_generator_html: '$metadata_generator_html'"
	echo "metadata_generator_pdf: '$metadata_generator_pdf'"
	# FIXME: debug

	# autodetection: check also Asciidoc
	if test "x$with_metadata_generator" = xdetect; then
		with_metadata_generator='asciidoctor'
		if test "x$metadata_generator_html" = x -a "x$enable_metadata_html" != xyes ||
			test "x$metadata_generator_pdf" = x -a "x$enable_metadata_pdf" != xyes; then
			backup_html="$metadata_generator_html"
			backup_pdf="$metadata_generator_pdf"
			LTP_CHECK_METADATA_GENERATOR_ASCIIDOC
			# prefer Asciidoctor if it's not worse than Asciidoc
			if test "x$metadata_generator_html" = x -o "x$enable_metadata_html" != xyes &&
				test "x$metadata_generator_pdf" = x -o "x$enable_metadata_pdf" != xyes; then
				with_metadata_generator='asciidoctor'
				metadata_generator_html="$backup_html"
				metadata_generator_pdf="$backup_pdf"
			fi
		fi
		AC_MSG_NOTICE(choosing $with_metadata_generator for metadata generation)
	fi

	if test "x$metadata_generator_html" = x; then
		AC_SUBST(WITH_METADATA_HTML, "no")
		if test "x$enable_metadata_html" != xyes; then
			AC_MSG_ERROR(html metadata generattion disabled due missing generator, specify correct generator with --with-metadata-generator=asciidoc|asciidoctor or disable this warning with --disable-metadata-html)
		fi
	else
		AC_SUBST(WITH_METADATA_HTML, "yes")
	fi

	if test "x$metadata_generator_pdf" = x; then
		AC_SUBST(WITH_METADATA_PDF, "no")
		if test "x$enable_metadata_pdf" != xyes; then
			AC_MSG_ERROR(pdf metadata generattion disabled due missing generator, specify correct generator with --with-metadata-generator=asciidoc|asciidoctor or disable this warning with --disable-metadata-pdf)
		fi
	else
		AC_SUBST(WITH_METADATA_PDF, "yes")
	fi
	AC_SUBST(WITH_METADATA, "yes")
	AC_SUBST(METADATA_GENERATOR, $with_metadata_generator)
else
	AC_SUBST(WITH_METADATA, "no")
	AC_SUBST(WITH_METADATA_HTML, "no")
	AC_SUBST(WITH_METADATA_PDF, "no")
	AC_MSG_NOTICE(metadata generation disabled)
	echo "DISABLED" # FIXME: debug
fi
])
