grammar walang;

walang: statement* EOF;

identifier:
	IntNumber
	| HexNumber
	| FloatNumber
	| Identifier; // TODO

type: Identifier;

// statement

statement:
	expressionStatement
	| declareStatement
	| assignStatement
	| blockStatement
	| ifStatement
	| whileStatement
	| breakStatement
	| continueStatement
	| returnStatement
	| functionStatement
	| classStatement;

// basis statement
expressionStatement: expression ';';
declareStatement:
	'let' Identifier (':' type)? '=' expression ';';
assignStatement: expression '=' expression ';';
returnStatement: 'return' expression ';';

// flow statement
blockStatement: '{' statement* '}';
ifStatement:
	'if' '(' expression ')' blockStatement (
		'else' (blockStatement | ifStatement)
	)?;
whileStatement: 'while' '(' expression ')' blockStatement;
breakStatement: 'break' ';';
continueStatement: 'continue' ';';

// function statement
parameter: Identifier ':' type;
parameterList: (parameter (',' parameter)*)?;
functionStatement:
	'function' Identifier '(' parameterList ')' (':' type)? blockStatement;

member: Identifier ':' type ';';
classStatement:
	'class' Identifier '{' (functionStatement | member)* '}';

// expression
prefixOperator: 'not' | '+' | '-';
binaryOperator:
	'*'
	| '/'
	| '%'
	| '+'
	| '-'
	| '<<'
	| '>>'
	| '<'
	| '>'
	| '<='
	| '>='
	| '=='
	| '!='
	| '&'
	| '^'
	| '|'
	| '&&'
	| '||';

prefixExpression: prefixOperator expression;

parenthesesExpression: '(' expression ')';

binaryExpressionRightWithOp:
	binaryOperator binaryExpressionRight;
binaryExpressionLeft:
	identifier
	| prefixExpression
	| parenthesesExpression
	// | binaryExpression | ternaryExpression
	| callExpression
	| memberExpression;
binaryExpressionRight:
	identifier
	| prefixExpression
	| parenthesesExpression
	| binaryExpression
	//| ternaryExpression
	| callExpression
	| memberExpression;
binaryExpression:
	binaryExpressionLeft binaryExpressionRightWithOp+;

ternaryExpressionBody: '?' expression ':' expression;
ternaryExpressionCondition:
	identifier
	| prefixExpression
	| parenthesesExpression
	| binaryExpression
	// | ternaryExpression
	| callExpression
	| memberExpression;
ternaryExpression:
	ternaryExpressionCondition ternaryExpressionBody+;

callOrMemberExpressionLeft: identifier | parenthesesExpression;
callExpressionRight: '(' (expression (',' expression)*)? ')';
memberExpressionRight: '.' Identifier;
callOrMemberExpressionRight:
	callExpressionRight
	| memberExpressionRight;
callExpression:
	callOrMemberExpressionLeft callOrMemberExpressionRight* callExpressionRight;
memberExpression:
	callOrMemberExpressionLeft callOrMemberExpressionRight* memberExpressionRight;

expression:
	identifier
	| prefixExpression
	| parenthesesExpression
	| binaryExpression
	| ternaryExpression
	| callExpression
	| memberExpression;

// Keyword

IF: 'if';
ELSE: 'else';
WHILE: 'while';
BREAK: 'break';
CONTINUE: 'continue';
LET: 'let';
NOT: 'not';
FUNCTION: 'function';
CLASS: 'class';
RETURN: 'return';

// Operator
LParenthesis: '(';
RParenthesis: ')';
LBrace: '{';
RBrace: '}';
LBracket: '[';
RBracket: ']';

Less: '<';
LessEqual: '<=';
Greater: '>';
GreaterEqual: '>=';
LeftShift: '<<';
RightShift: '>>';

Plus: '+';
Minus: '-';
Star: '*';
Div: '/';
Mod: '%';

And: '&';
Or: '|';
AndAnd: '&&';
OrOr: '||';
Caret: '^';

Question: '?';
Colon: ':';
Semi: ';';

Assign: '=';

Equal: '==';
NotEqual: '!=';

Dot: '.';

// Token

Identifier: NonDigit ( NonDigit | Digit)*;

IntNumber: Digit+;
HexNumber: '0' [xX] HexDigit+;
FloatNumber: Digit+ '.' Digit+;

fragment Digit: [0-9];
fragment HexDigit: [0-9a-fA-F];

fragment NonDigit:
	[a-zA-Z_]
	| '\\u' HexDigit HexDigit HexDigit HexDigit
	| '\\U' HexDigit HexDigit HexDigit HexDigit HexDigit HexDigit HexDigit HexDigit;

Whitespace: [ \t]+ -> skip;
Newline: ( '\r' '\n'? | '\n') -> skip;
BlockComment: '/*' .*? '*/' -> skip;
LineComment: '//' ~[\r\n]* -> skip;

