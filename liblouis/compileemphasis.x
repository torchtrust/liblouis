static int
compileEmphasisOpcode (FileInfo * nested, TranslationTableOpcode opcode)
{
  /*Compile the operands of an emphasis opcode */
  /* Note that much of the code for multipass opcodes is repurposed here*/
  TranslationTableOffset ruleOffset = 0;
  TranslationTableRule *rule = NULL;
  int k;
  int kk = 0;
  passRuleChars.length = 0;
  passNested = nested;
  passOpcode = opcode;
/* passHoldString and passLine are static variables declared 
 * previously.*/
  passLinepos = 0;
  passHoldString.length = 0;
  for (k = nested->linepos; k < nested->linelen; k++)
    passHoldString.chars[passHoldString.length++] = nested->line[k];
  parseChars (passNested, &passLine, &passHoldString);
  if (!addRule (passNested, opcode, &passRuleChars, &passRuleDots, 0, 0))
    return 0;
  return 1;
}
