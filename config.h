static const char content_type[]	= "application/xhtml+xml";
static const char title[]		= "clog";
static const char slogan[]		= "the fastest and simplest blog on earth";
static const char encoding[]		= "UTF-8";
static const char cssfile[]		= "bash.css";

static const char root_dir[]		= "/homes/y0925294/Desktop";
static const char mainfile[]		= "clog.out?dir=";

static Filehandler filehandler[] = {
	/* name		filter		function*/
	{"TEXT",	isText,		showText},
/*	{"MARKDOWN",	isMarkdown,	showMarkdown},
	{"PDF",		isPDF,		showPDF}, */
	{"PICTURES",	isPicture,	showPicture}
};
