grammar walang;

walang: statement* EOF;

identifier: IntNumber | HexNumber | Identifier; // TODO

// statement

statement:
	expressionStatement
	| declareStatement
	| assignStatement
	| blockStatement
	| ifStatement
	| whileStatement
	| breakStatement
	| continueStatement;

expressionStatement: expression ';';

declareStatement: 'let' Identifier '=' expression ';';

assignStatement: expression '=' expression ';';

blockStatement: '{' statement* '}';
ifStatement:
	'if' '(' expression ')' blockStatement (
		'else' (blockStatement | ifStatement)
	)?;
whileStatement: 'while' '(' expression ')' blockStatement;
breakStatement: 'break' ';';
continueStatement: 'continue' ';';

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
	| callExpression;
binaryExpressionRight:
	identifier
	| prefixExpression
	| parenthesesExpression
	| binaryExpression
	//| ternaryExpression
	| callExpression;
binaryExpression:
	binaryExpressionLeft binaryExpressionRightWithOp+;

ternaryExpressionBody: '?' expression ':' expression;
ternaryExpressionCondition:
	identifier
	| prefixExpression
	| parenthesesExpression
	| binaryExpression
	// | ternaryExpression
	| callExpression;
ternaryExpression:
	ternaryExpressionCondition ternaryExpressionBody+;

callExpressionRight: '(' expression (',' expression)* ')';
callExpression: identifier callExpressionRight+;

expression:
	identifier
	| prefixExpression
	| parenthesesExpression
	| binaryExpression
	| ternaryExpression
	| callExpression;

// Keyword

IF: 'if';
ELSE: 'else';
WHILE: 'while';
BREAK: 'break';
CONTINUE: 'continue';
LET: 'let';
NOT: 'not';

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

IntNumber: [0-9]+;
HexNumber: '0' [xX] HexDigit+;

fragment Digit: [0-9a-fA-F];
fragment HexDigit: [0-9a-fA-F];

fragment NonDigit:
	[a-zA-Z_]
	| '\\u' HexDigit HexDigit HexDigit HexDigit
	| '\\U' HexDigit HexDigit HexDigit HexDigit HexDigit HexDigit HexDigit HexDigit;

Whitespace: [ \t]+ -> skip;
Newline: ( '\r' '\n'? | '\n') -> skip;
BlockComment: '/*' .*? '*/' -> skip;
LineComment: '//' ~[\r\n]* -> skip;

