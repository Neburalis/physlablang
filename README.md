## Примерный РБНФ:
```
program ::= report

report ::=
    "ЛАБОРАТОРНАЯ РАБОТА" literal
    annotation
    theoretical_background
    experimental_procedure
    results_discussion
    conclusion
    "AI generated for reference only"

annotation ::=
    "АННОТАЦИЯ"
    "ЦЕЛЬ:" literal
    ("ЗАДАЧИ:" literal)?
    ("МЕТОДЫ:" literal)?
    ("ОЖИДАЕМЫЕ РЕЗУЛЬТАТЫ:" literal)?
    "КОНЕЦ АННОТАЦИИ"

theoretical_background ::=
    "ТЕОРЕТИЧЕСКИЕ СВЕДЕНИЯ"
    (literal* function_declaration+)+
    "КОНЕЦ ТЕОРИИ"

experimental_procedure ::=
    "ХОД РАБОТЫ"
    operators
    "КОНЕЦ РАБОТЫ"

results_discussion ::=
    "ОБСУЖДЕНИЕ РЕЗУЛЬТАТОВ"
    operators
    "КОНЕЦ РЕЗУЛЬТАТОВ"

conclusion ::=
    "ВЫВОДЫ"
    literal
    "КОНЕЦ ВЫВОДОВ" | "КОНЕЦ ВЫВОДА"

function_declaration ::=
    "ФОРМУЛА" identifier "(" identifier_list ")" operators "КОНЕЦ ФОРМУЛЫ"

operators ::= operator+

operator ::=
    variable_declaration
    | assignment
    | function_call
    | conditional
    | loop
    | io_statement
    | return_statement

variable_declaration ::=
    "ВЕЛИЧИНА" identifier
    | "ВЕЛИЧИНА" identifier "=" expression

assignment ::=
    identifier "=" expression

conditional ::=
    "ЕСЛИ" expression "ТО" operators
    | "ЕСЛИ" expression "ТО" operators "ИНАЧЕ" operators

loop ::=
    "ПОКА" expression "ПОВТОРЯЕМ" operators "СТОП"
    | "ПОВТОРЯЕМ" operators "ПОКА" expression "СТОП"

io_statement ::=
    "ВЫВЕСТИ" expression
    | "ИЗМЕРИТЬ" identifier
    | "ПОКАЗАТЬ" expression

return_statement ::= "ВОЗВРАТИТЬ" expression

expression ::=
    assignment
    | function_call
    | logical_expression
    | math_expression
    | comparison_expression
    | primary_expression
    | string

logical_expression ::=
    comparison_expression (("И" | "ИЛИ") comparison_expression)*

comparison_expression ::=
    math_expression ("==" | "!=" | "<" | ">" | "<=" | ">=") math_expression

math_expression ::=
    term (("+" | "-") term)*

term ::=
    factor (("*" | "/" | "%") factor)*

factor ::=
    number
    | identifier
    | function_call
    | "(" expression ")"
    | internal_func

internal_func ::=
    unary_func "(" expression ")"
    | binary_func "(" expression, expression ")"

unary_func ::=
    "sin" | "cos" | "tg" | "tan" | "ctg"
    | "asin" | "arcsin" | "acos" | "arccos"
    | "atan" | "arctan" | "arctg"
    | "actg" | "arcctg"
    | "sqrt"
    | "ln"

binary_func ::=
    "pow"

function_call ::=
    identifier arguments
    | identifier "ПРИМЕНЯЕМ" arguments
    | identifier "ВЫЧИСЛЯЕМ" arguments

arguments ::= expression ("," expression)*

identifier_list ::= identifier ("," identifier)*

identifier ::= [a-zA-Z_][a-zA-Z0-9_]*

number ::= [0-9]+ ("." [0-9]+)?

string ::= multi_line_string | one_line_string

multi_line_string ::= "\"" literal+ "\""
one_line_string ::= "\"\"\"" literal "\"\"\""

literal ::= [^\"^\n]*
```

## Правила для лексического анализа
Все ключевые слова могут писаться как в верхнем так и в нижнем регистре. Для переменных (идентификаторов) регистр имеет значение.
```
комментарий (// text_literal) -> ничто
"(" -> DELIMITER.PAR_OPEN
")" -> DELIMITER.PAR_CLOSE
"\"" -> DELIMITER.QUOTE
"," -> DELIMITER.COMA
":" -> DELIMITER.COLON
"==" -> OPERATOR.EQ
"!=" -> OPERATOR.NEQ
"=" -> OPERATOR.ASSIGNMENT
"<" -> OPERATOR.BELOW
">" -> OPERATOR.ABOVE
"<=" -> OPERATOR.BELOW_EQ
">=" -> OPERATOR.ABOVE_EQ
"+" -> OPERATOR.ADD
"-" -> OPERATOR.SUB
"*" -> OPERATOR.MUL
"/" -> OPERATOR.DIV
"^" -> OPERATOR.POW
"ln" -> OPERATOR.LN
"sin" -> OPERATOR.SIN
"cos" -> OPERATOR.COS
"tan" -> OPERATOR.TAN
"tg" -> OPERATOR.TAN
"ctg" -> OPERATOR.CTG
"asin" -> OPERATOR.ASIN
"arcsin" -> OPERATOR.ASIN
"acos" -> OPERATOR.ACOS
"arccos" -> OPERATOR.ACOS
"atan" -> OPERATOR.ATAN
"arctan" -> OPERATOR.ATAN
"arctg" -> OPERATOR.ATAN
"actg" -> OPERATOR.ACTG
"arcctg" -> OPERATOR.ACTG
"sqrt" -> OPERATOR.SQRT
"and" -> OPERATOR.AND
"И" -> OPERATOR.AND
"or" -> OPERATOR.OR
"или" -> OPERATOR.OR
"не" -> OPERATOR.NOT
"not" -> OPERATOR.NOT
"ЛАБОРАТОРНАЯ РАБОТА" -> KEYWORD.LAB
"АННОТАЦИЯ" -> KEYWORD.ANNOTATION
"КОНЕЦ АННОТАЦИИ" -> KEYWORD.END_ANNOTATION
"ТЕОРЕТИЧЕСКИЕ СВЕДЕНИЯ" -> KEYWORD.THEORETICAL
"КОНЕЦ ТЕОРИИ" -> KEYWORD.END_THEORETICAL
"ХОД РАБОТЫ" -> KEYWORD.EXPERIMENTAL
"КОНЕЦ РАБОТЫ" -> KEYWORD.END_EXPERIMENTAL
"ОБСУЖДЕНИЕ РЕЗУЛЬТАТОВ" -> KEYWORD.RESULTS
"КОНЕЦ РЕЗУЛЬТАТОВ" -> KEYWORD.END_RESULTS
"ВЫВОДЫ" -> KEYWORD.CONCLUSION
"КОНЕЦ ВЫВОДОВ" -> KEYWORD.END_CONCLUSION
"КОНЕЦ ВЫВОДА" -> KEYWORD.END_CONCLUSION
"ЦЕЛЬ" -> KEYWORD.GOAL_LITERAL
"ПОВТОРЯЕМ" -> KEYWORD.WHILE
"ПОКА" -> KEYWORD.WHILE_CONDITION
"СТОП" -> KEYWORD.END_WHILE
"ФОРМУЛА" -> KEYWORD.FORMULA
"КОНЕЦ ФОРМУЛЫ" -> KEYWORD.END_FORMULA
"РАССЧИТЫВАЕТСЯ ИЗ" -> KEYWORD.FUNC_CALL
"ВЕЛИЧИНА" -> KEYWORD.VAR_DECLARATION
"ЕСЛИ" -> KEYWORD.IF
"ТО" -> KEYWORD.THEN
"ИНАЧЕ" -> KEYWORD.ELSE
"ВЫВЕСТИ" -> KEYWORD.OUT
"ПОКАЗАТЬ" -> KEYWORD.OUT
"ИЗМЕРИТЬ" -> KEYWORD.IN
"ВОЗВРАТИТЬ" -> KEYWORD.RETURN
"ПРИМЕНЯЕМ" -> KEYWORD.FUNC_CALL
"ВЫЧИСЛЯЕМ" -> KEYWORD.FUNC_CALL

число -> NUMBER.значение
идентификатор -> идентификатор
```

## Построение AST

### Циклы

Для отображения дерева в текстовом виде используется полная префиксная скобочная запись

``` physlab
ПОКА expression ПОВТОРЯЕМ
    operators
СТОП
```
лексическим анализом будет преобразовано в:
```
KEYWORD.WHILE_CONDITION
expression
KEYWORD.WHILE
operators
KEYWORD.END_WHILE
```

на основе этого грамматика должна сгенерировать:
```
(KEYWORD.WHILE (expression) (operators))
```
где expression предполагает дерево условия, а operators - дерево тела цикла

То есть в левом поддереве цикла лежит условие, а в правом само тело.

**Для do-while цикла**:

``` physlab
ПОВТОРЯЕМ
    operators
ПОКА expression СТОП
```
После лексера получится:
```
KEYWORD.WHILE
operators
KEYWORD.WHILE_CONDITION
expression
KEYWORD.END_WHILE
```
грамматика должна сгенерировать:
```
(KEYWORD.DO_WHILE (expression) (operators))
```
То есть в левом поддереве цикла также лежит условие, а в правом само тело. (При это нужно заменить значение токена с KEYWORD.WHILE на KEYWORD.DO_WHILE)

### условный оператор

```physlab
ЕСЛИ expr ТО op
```
```physlab
ЕСЛИ expr ТО op1 ИНАЧЕ op2
```
Токены:
```
"ЕСЛИ" -> KEYWORD.IF
expr
"ТО" -> KEYWORD.THEN
op
```
```
"ЕСЛИ" -> KEYWORD.IF
expr
"ТО" -> KEYWORD.THEN
op1
"ИНАЧЕ" -> KEYWORD.ELSE
op2
```
дерево:
```
(IF (expr) (KEYWORD.THEN (op1) (op2)))
```

Если блока else нет, то вместо op2 -> nil

Соответственно цепочка if-else-if-else:
```physlab
ЕСЛИ expr1 ТО
    op1
ИНАЧЕ ЕСЛИ expr 2 ТО
    op2
ИНАЧЕ op3
```
создаст дерево:
```
(
    IF (expr1) (
        THEN (op1) (
            IF (expr2) (
                THEN (
                    op2
                ) (
                    op3
                )
            )
        )
    )
)
```
т.к. сам условный оператор тоже является оператором и этот случай не требует никакой дополнительной логики

### Идентификаторы и литералы
записываются в дерево в виде:
```
(
    "LITERAL" // Заковыченная строка
    nil nil
)
```

### Оператор связи

Оператор CONNECTOR не создается лексическим анализом, а должен создаваться грамматикой

Он необходим т.к. дерево бинарное

То есть при парсинге `operators ::= operator+` грамматика должна создавать CONNECTOR для каждого считанного оператора

Например:
```physlab
ВЕЛИЧИНА x = 0
ЕСЛИ x < 5 ТО x = x + 1
```

дерево:
```
(
    ;
    (
        ;
        (
            KEYWORD.VAR_DECLARATION
            (
                "x"
                nil
                nil
            )
            nil
        )
        (
            OPERATOR.ASSIGNMENT
            (
                "x"
                nil
                nil
            )
            (
                0
                nil
                nil
            )
        )
    )
    (
        IF
        (
            <
            (
                "x"
                nil
                nil
            )
            (
                3
                nil
                nil
            )
        )
        (
            THEN
            (
                ASSIGNMENT
                (
                    "x"
                    nil
                    nil
                )
                (
                    +
                    (
                        "x"
                        nil
                        nil
                    )
                    (
                        1
                        nil
                        nil
                    )
                )
            )
            nil
        )
    )
)
```

### Присваивание
```physlab
id = expr
```

Дерево:
```
(
    ASSIGNMENT (id) (expr)
)
```

### Объявление переменной
```physlab
ВЕЛИЧИНА id
```
дерево:
```
(VAR_DECLARATION (id) nil)
```

```
ВЕЛИЧИНА id = expr
```
дерево
```
(
    ;
    (VAR_DECLARATION (id) nil)
    (= (id) (expr))
)
```

### Объявление функций
```physlab
ФОРМУЛА sum (a, b, c)
    ВЕЛИЧИНА result = a + b + c
    ВОЗВРАТИТЬ result
КОНЕЦ ФОРМУЛЫ
```

```
(
    ("sum" nil nil)
    (
        ,
        ("a" nil nil)
        (
            ,
            ("b" nil nil)
            (
                ,
                ("c" nil nil)
                nil
            )
        )
    )
    (
        ,
        (
            ,
            (
                VAR_DECLARATION
                ("result" nil nil)
                nil
            )
            (
                =
                ("result" nil nil)
                (
                    +
                    ("a" nil nil)
                    (
                        +
                        ("b" nil nil)
                        ("c" nil nil)
                    )
                )
            )
        )
        (RETURN ("result" nil nil) nil)
    )
)
```

### Вызов функций
```
(
    FUNC_CALL
    ("sum" nil nil)
    (
        ,
        (1 nil nil)
        (
            ,
            (2 nil nil)
            (
                ,
                (3 nil nil)
                nil
            )
        )
    )
)
```

структура аргументов аналогично объявлению, но вместо id лежат expr

### Операторы ввода/вывода и встроенные функции

Операторы ввода/вывода и встроенные алгебраические функции работают также как и простейшие математические функции (но в основном унарные а не бинарные)

`ИЗМЕРИТЬ x -> (IN  ("x" nil nil) nil)`

`sin (x) ->    (SIN ("x" nil nil) nil)`

`ВЫВЕСТИ x ->  (OUT ("x" nil nil) nil)`

### Операторы и выражения лежат "как есть"
`x >= 0 И x + 1 < 15` ->
```
(
    +
    (
        >=
        ("x" nil nil)
        (0 nil nil)
    )
    (
        <
        (
            +
            ("x" nil nil)
            (1 nil nil)
        )
        (15 nil nil)
    )
)
```


### устройство AST в целом

Корнем дерева программы является OPERATOR.CONNECTOR. В его левом поддереве лежат определения функций (формул) через DELIMITER.COMA (аналогично тому как параметры лежат в определении функции). В правом поддереве лежит тело программы.