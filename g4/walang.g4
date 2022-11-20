grammar walang;

walang: statement* EOF;

identifier: IntNumber | HexNumber | Identifier; // TODO

// statement

statement:
	expressionStatement
	| declareStatement
	| assignStatement
	| ifStatement
	| whileStatement;

expressionStatement: expression ';';

declareStatement: 'let' Identifier '=' expression ';';

assignStatement: expression '=' expression ';';

ifStatement:
	'if' '{' statement+ '}' ('else' 'if' '{' statement+ '}')* (
		'else' '{' statement+ '}'
	)?;
whileStatement: 'while' '(' expression ')' '{' statement+ '}';

// expression

prefixOperator: 'not';
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
	| '||'
	| '.';

prefixExpression: prefixOperator expression;

parenthesesExpression: '(' expression ')';

binaryExpressionRight: binaryOperator expression;
binaryExpression: identifier binaryExpressionRight+;

ternaryExpressionRight: '?' expression ':' expression;
ternaryExpression: identifier ternaryExpressionRight+;

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

