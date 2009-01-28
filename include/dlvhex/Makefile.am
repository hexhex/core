AUTOMAKE_OPTIONS = gnu

# remove $prefix/include/dlvhex directory after uninstall
uninstall-hook:
	-test -d $(DESTDIR)$(pkgincludedir) && rmdir $(DESTDIR)$(pkgincludedir); exit 0


# including headers for make dist
noinst_HEADERS = \
	ASPSolver.h \
	ASPSolver.tcc \
	DLVresultFlexLexer.h \
	DLVresultParserDriver.h \
	GraphProcessor.h \
	HexFlexLexer.h \
	HexParserDriver.h \
	ParserDriver.h \
	PredefinedNames.h \
	RuleMLOutputBuilder.h \
	SafetyChecker.h \
	State.h \
	TextOutputBuilder.h \
	URLBuf.h

pkginclude_HEADERS = \
	AggregateAtom.h \
	AnswerSet.h \
	Atom.h \
	Atom.tcc \
	AtomNode.h \
	AtomSet.h \
	AtomSetFunctions.h \
	BaseAtom.h \
	BaseLiteral.h \
	BaseRule.h \
	BaseQuery.h \
	BaseVisitor.h \
	Body.h \
	BoostComponentFinder.h \
	ComponentFinder.h \
	Component.h \
	Constraint.h \
	DepGraphDirector.h \
	DepGraphDirector.tcc \
	Dependency.h \
	DependencyGraph.h \
	DLVProcess.h \
	Error.h \
	EvaluateExtatom.h \
	ExternalAtom.h \
	globals.h \
	GraphBuilder.h \
	Head.h \
	HexDepGraph.h \
	Literal.h \
	Literal.tcc \
	ModelGenerator.h \
	NamesTable.h \
	NegationTraits.h \
	NodeGraph.h \
	NullVisitor.h \
	OutputBuilder.h \
	PlatformDefinitions.h \
	PluginContainer.h \
	PluginInterface.h \
	PrintVisitor.h \
	PredefinedNames.h \
	ProgramNode.h \
	Process.h \
	ProcessBuf.h \
	Program.h \
	ProgramCtx.h \
	QueryTraits.h \
	ResultContainer.h \
	Rule.h \
	Term.h \
	WeakConstraint.h
