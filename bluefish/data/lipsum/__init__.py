from cStringIO import StringIO
import base64
import gzip
import math
import os
import random
import re
import pkg_resources

# Delimiters that mark ends of sentences
_DELIMITERS_SENTENCES = ['.', '?', '!']

# Delimiters which do not form parts of words (i.e. "hello," is the word
# "hello" with a comma next to it)
_DELIMITERS_WORDS = [','] + _DELIMITERS_SENTENCES

_NEWLINE = os.linesep

_DEFAULT_SAMPLE = pkg_resources.resource_string('lipsum', 'data/sample.txt')

_DEFAULT_DICT = pkg_resources.resource_string('lipsum', 'data/dictionary.txt').split()

def _split_paragraphs(text):
    """
    Splits a piece of text into paragraphs, separated by empty lines.

    """
    lines = text.splitlines()
    paragraphs = [[]]
    for line in lines:
        if len(line.strip()) > 0:
            paragraphs[-1] += [line]
        elif len(paragraphs[-1]) > 0:
            paragraphs.append([])
    paragraphs = map(' '.join, paragraphs)
    paragraphs = map(str.strip, paragraphs)
    paragraphs = filter(lambda s : s != '', paragraphs)
    return paragraphs

def _split_sentences(text):
    """
    Splits a piece of text into sentences, separated by periods, question 
    marks and exclamation marks.

    """
    sentence_split = ''
    for delimiter in _DELIMITERS_SENTENCES:
        sentence_split += '\\' + delimiter
    sentence_split = '[' + sentence_split + ']'
    sentences = re.split(sentence_split, text.strip())
    sentences = map(str.strip, sentences)
    sentences = filter(lambda s : s != '', sentences)
    return sentences

def _split_words(text):
    """
    Splits a piece of text into words, separated by whitespace.
    
    """
    return text.split()

def _mean(values):
    return sum(values) / float(max(len(values), 1))

def _variance(values):
    squared = map(lambda x : x**2, values)
    return _mean(squared) - _mean(values)**2

def _sigma(values):
    return math.sqrt(_variance(values))

def _choose_closest(values, target):
    """
    Find the number in the list of values that is closest to the target. 
    Prefer the first in the list.
    
    """
    closest = values[0]
    for value in values:
        if abs(target - value) < abs(target - closest):
            closest = value

    return closest

def _get_word_info(word):
    longest = (word, "")

    for delimiter in _DELIMITERS_WORDS:
        if len(delimiter) > len(longest[1]) and word.endswith(delimiter):
            word = word.rpartition(delimiter)
            longest = (word[0], word[1])

    return (len(longest[0]), longest[1])


class InvalidDictionaryError(Exception):
    def __str__(self):
        return ('The dictionary must be a list of one or more words.')

class InvalidSampleError(Exception):
    def __str__(self):
        return ('The sample text must contain one or more empty-line '
            'delimited paragraphs, and each paragraph must contain one or '
            'more period, question mark, or exclamation mark delimited '
            'sentences.')

class Generator(object):
    """
    Generates random strings of "lorem ipsum" text.

    Markov chains are used to generate the random text based on the analysis 
    of a sample text. In the analysis, only paragraph, sentence and word 
    lengths, and some basic punctuation matter -- the actual words are 
    ignored. A provided list of words is then used to generate the random text,
    so that it will have a similar distribution of paragraph, sentence and word
    lengths.

    """

    # Words that can be used in the generated output
    # Maps a word-length to a list of words of that length
    __words = {}

    # Chains of three words that appear in the sample text
    # Maps a pair of word-lengths to a third word-length and an optional 
    # piece of trailing punctuation (for example, a period, comma, etc.)
    __chains = {}

    # Pairs of word-lengths that can appear at the beginning of sentences
    __starts = []

    # Sample that the generated text is based on
    __sample = ""

    # Statistics for sentence and paragraph generation
    __sentence_mean = 0
    __sentence_sigma = 0
    __paragraph_mean = 0
    __paragraph_sigma = 0

    # Last calculated statistics, in case they are overwritten by user
    __generated_sentence_mean = 0
    __generated_sentence_sigma = 0
    __generated_paragraph_mean = 0
    __generated_paragraph_sigma = 0

    def __init__(self, sample=_DEFAULT_SAMPLE, dictionary=_DEFAULT_DICT):
        """
        Initialises a generator that will use the provided sample text and 
        dictionary to produce sentences.

        """
        self.sample = sample
        self.dictionary = dictionary

    def __set_sentence_mean(self, mean):
        if mean < 0:
            raise ValueError('Mean sentence length must be non-negative.')
        self.__sentence_mean = mean

    def __set_sentence_sigma(self, sigma):
        if sigma < 0:
            raise ValueError('Standard deviation of sentence length must be '
                'non-negative.')
        self.__sentence_sigma = sigma

    def __set_paragraph_mean(self, mean):
        if mean < 0:
            raise ValueError('Mean paragraph length must be non-negative.')
        self.__paragraph_mean = mean

    def __set_paragraph_sigma(self, sigma):
        if sigma < 0:
            raise ValueError('Standard deviation of paragraph length must be '
                'non-negative.')
        self.__paragraph_sigma = sigma

    def __get_sentence_mean(self):
        """
        A non-negative value determining the mean sentence length (in words)
        of generated sentences. Is changed to match the sample text when the
        sample text is updated.
        """
        return self.__sentence_mean

    def __get_sentence_sigma(self):
        """
        A non-negative value determining the standard deviation of sentence
        lengths (in words) of generated sentences. Is changed to match the
        sample text when the sample text is updated.
        """
        return self.__sentence_sigma

    def __get_paragraph_mean(self):
        """
        A non-negative value determining the mean paragraph length (in
        sentences) of generated sentences. Is changed to match the sample text
        when the sample text is updated.
        """
        return self.__paragraph_mean

    def __get_paragraph_sigma(self):
        """
        A non-negative value determining the standard deviation of paragraph
        lengths (in sentences) of generated sentences. Is changed to match the
        sample text when the sample text is updated.
        """
        return self.__paragraph_sigma

    sentence_mean = property(__get_sentence_mean, __set_sentence_mean)
    sentence_sigma = property(__get_sentence_sigma, __set_sentence_sigma)
    paragraph_mean = property(__get_paragraph_mean, __set_paragraph_mean)
    paragraph_sigma = property(__get_paragraph_sigma, __set_paragraph_sigma)

    def __generate_chains(self, sample):
        """
        Generates the __chains and __starts values required for sentence generation.
        """
        words = _split_words(sample)
        if len(words) <= 0:
            raise InvalidSampleError

        word_info = map(_get_word_info, words)

        previous = (0, 0)
        chains = {}
        starts = [previous]

        for pair in word_info:
            if pair[0] == 0:
                continue
            chains.setdefault(previous, []).append(pair)
            if pair[1] in _DELIMITERS_SENTENCES:
                starts.append(previous)
            previous = (previous[1], pair[0])

        if len(chains) > 0:
            self.__chains = chains
            self.__starts = starts
        else:
            raise InvalidSampleError

    def __generate_statistics(self, sample):
        """
        Calculates the mean and standard deviation of sentence and paragraph lengths.
        """
        self.__generate_sentence_statistics(sample)
        self.__generate_paragraph_statistics(sample)
        self.reset_statistics()

    def __generate_sentence_statistics(self, sample):
        """
        Calculates the mean and standard deviation of the lengths of sentences 
        (in words) in a sample text.
        """
        sentences = filter(lambda s : len(s.strip()) > 0, _split_sentences(sample))
        sentence_lengths = map(len, map(_split_words, sentences))
        self.__generated_sentence_mean = _mean(sentence_lengths)
        self.__generated_sentence_sigma = _sigma(sentence_lengths)

    def __generate_paragraph_statistics(self, sample):
        """
        Calculates the mean and standard deviation of the lengths of paragraphs
        (in sentences) in a sample text.
        """
        paragraphs = filter(lambda s : len(s.strip()) > 0, _split_paragraphs(sample))
        paragraph_lengths = map(len, map(_split_sentences, paragraphs))
        self.__generated_paragraph_mean = _mean(paragraph_lengths)
        self.__generated_paragraph_sigma = _sigma(paragraph_lengths)

    def reset_statistics(self):
        """
        Returns the values of sentence_mean, sentence_sigma, paragraph_mean,
        and paragraph_sigma to their values as calculated from the sample
        text.
        """
        self.sentence_mean = self.__generated_sentence_mean
        self.sentence_sigma = self.__generated_sentence_sigma
        self.paragraph_mean = self.__generated_paragraph_mean
        self.paragraph_sigma = self.__generated_paragraph_sigma

    def __get_sample(self):
        """
        The sample text that generated sentences are based on.

        Sentences are generated so that they will have a similar distribution 
        of word, sentence and paragraph lengths and punctuation.

        Sample text should be a string consisting of a number of paragraphs, 
        each separated by empty lines. Each paragraph should consist of a 
        number of sentences, separated by periods, exclamation marks and/or
        question marks. Sentences consist of words, separated by white space.
        """
        return self.__sample

    def __set_sample(self, sample):
        self.__sample = sample
        self.__generate_chains(sample)
        self.__generate_statistics(sample)

    def __get_dictionary(self):
        """
        The list of words that generated sentences are made of.

        The dictionary should be a list of one or more words, as strings.
        """
        dictionary = []
        map(dictionary.extend, self.__words.values())
        return dictionary

    def __set_dictionary(self, dictionary):
        words = {}

        for word in dictionary:
            try:
                word = str(word)
                words.setdefault(len(word), set()).add(word)
            except TypeError:
                continue

        if len(words) > 0:
            self.__words = words
        else:
            raise InvalidDictionaryError

    sample = property(__get_sample, __set_sample)
    dictionary = property(__get_dictionary, __set_dictionary)

    def __choose_random_start(self):
        starts = set(self.__starts)
        chains = set(self.__chains.keys())
        valid_starts = list(chains.intersection(starts))
        return random.choice(valid_starts)

    def generate_sentence(self, start_with_lorem=False):
        """
        Generates a single sentence, of random length.

        If start_with_lorem=True, then the sentence will begin with the
        standard "Lorem ipsum..." first sentence.
        """
        if len(self.__chains) == 0 or len(self.__starts) == 0:
            raise InvalidSampleError

        if len(self.__words) == 0:
            raise InvalidDictionaryError

        # The length of the sentence is a normally distributed random variable.
        sentence_length = random.normalvariate(self.sentence_mean, \
            self.sentence_sigma)
        sentence_length = max(int(round(sentence_length)), 1)

        sentence = []
        previous = ()

        word_delimiter = '' # Defined here in case while loop doesn't run

        # Start the sentence with "Lorem ipsum...", if desired
        if start_with_lorem:
            lorem = "lorem ipsum dolor sit amet, consecteteur adipiscing elit"
            lorem = lorem.split()
            sentence += lorem[:sentence_length]
            last_char = sentence[-1][-1]
            if last_char in _DELIMITERS_WORDS:
                word_delimiter = last_char

        # Generate a sentence from the "chains"
        while len(sentence) < sentence_length:
            # If the current starting point is invalid, choose another randomly
            if (not self.__chains.has_key(previous)):
                previous = self.__choose_random_start()

            # Choose the next "chain" to go to. This determines the next word
            # length we'll use, and whether there is e.g. a comma at the end of
            # the word.
            chain = random.choice(self.__chains[previous])
            word_length = chain[0]

            # If the word delimiter contained in the chain is also a sentence
            # delimiter, then we don't include it because we don't want the
            # sentence to end prematurely (we want the length to match the
            # sentence_length value).
            if chain[1] in _DELIMITERS_SENTENCES:
                word_delimiter = ''
            else:
                word_delimiter = chain[1]

            # Choose a word randomly that matches (or closely matches) the
            # length we're after.
            closest_length = _choose_closest(
                    self.__words.keys(),
                    word_length)
            word = random.choice(list(self.__words[closest_length]))

            sentence += [word + word_delimiter]
            previous = (previous[1], word_length)

        # Finish the sentence off with capitalisation, a period and
        # form it into a string
        sentence = ' '.join(sentence)
        sentence = sentence.capitalize()
        sentence = sentence.rstrip(word_delimiter) + '.'

        return sentence

    def generate_paragraph(self, start_with_lorem=False):
        """
        Generates a single lorem ipsum paragraph, of random length.

        If start_with_lorem=True, then the paragraph will begin with the
        standard "Lorem ipsum..." first sentence.
        """
        paragraph = []

        # The length of the paragraph is a normally distributed random variable.
        paragraph_length = random.normalvariate(self.paragraph_mean, \
            self.paragraph_sigma)
        paragraph_length = max(int(round(paragraph_length)), 1)

        # Construct a paragraph from a number of sentences.
        while len(paragraph) < paragraph_length:
            sentence = self.generate_sentence(
                start_with_lorem = (start_with_lorem and len(paragraph) == 0)
                )
            paragraph += [sentence]

        # Form the paragraph into a string.
        paragraph = ' '.join(paragraph)

        return paragraph


class MarkupGenerator(Generator):
    """
    Provides a number of methods for producing "lorem ipsum" text with
    varying formats.

    """
    def __generate_markup(self, begin, end, between, quantity,
        start_with_lorem, function):
        """
        Generates multiple pieces of text, with begin before each piece, end
        after each piece, and between between each piece. Accepts a function
        that returns a string.
        """
        text = []

        while len(text) < quantity:
            part = function(
                    start_with_lorem = (start_with_lorem and len(text) == 0)
                    )
            part = begin + part + end
            text += [part]

        text = between.join(text)
        return text

    def __generate_markup_paragraphs(self, begin_paragraph, end_paragraph,
        between_paragraphs, quantity, start_with_lorem=False):
        return self.__generate_markup(
                begin_paragraph,
                end_paragraph,
                between_paragraphs,
                quantity,
                start_with_lorem,
                self.generate_paragraph)

    def __generate_markup_sentences(self, begin_sentence, end_sentence,
        between_sentences, quantity, start_with_lorem=False):
        return self.__generate_markup(
                begin_sentence,
                end_sentence,
                between_sentences,
                quantity,
                start_with_lorem,
                self.generate_sentence)

    def generate_paragraphs_plain(self, quantity, start_with_lorem=False):
        """Generates a number of paragraphs, separated by empty lines."""
        return self.__generate_markup_paragraphs(
                begin_paragraph='',
                end_paragraph='',
                between_paragraphs=_NEWLINE * 2,
                quantity=quantity,
                start_with_lorem=start_with_lorem
                )

    def generate_sentences_plain(self, quantity, start_with_lorem=False):
        """Generates a number of sentences."""
        return self.__generate_markup_sentences(
                begin_sentence='',
                end_sentence='',
                between_sentences=' ',
                quantity=quantity,
                start_with_lorem=start_with_lorem
                )

    def generate_paragraphs_html_p(self, quantity, start_with_lorem=False):
        """
        Generates a number of paragraphs, with each paragraph
        surrounded by HTML pararaph tags.
        """
        return self.__generate_markup_paragraphs(
                begin_paragraph='<p>' + _NEWLINE + '\t',
                end_paragraph=_NEWLINE + '</p>',
                between_paragraphs=_NEWLINE,
                quantity=quantity,
                start_with_lorem=start_with_lorem
                )

    def generate_sentences_html_p(self, quantity, start_with_lorem=False):
        """
        Generates a number of sentences, with each sentence
        surrounded by HTML pararaph tags.
        """
        return self.__generate_markup_sentences(
                begin_sentence='<p>' + _NEWLINE + '\t',
                end_sentence=_NEWLINE + '</p>',
                between_sentences=_NEWLINE,
                quantity=quantity,
                start_with_lorem=start_with_lorem
                )

    def generate_paragraphs_html_li(self, quantity, start_with_lorem=False):
        """Generates a number of paragraphs, separated by empty lines."""
        output = self.__generate_markup_paragraphs(
                begin_paragraph='\t<li>\n\t\t',
                end_paragraph='\n\t</li>',
                between_paragraphs=_NEWLINE,
                quantity=quantity,
                start_with_lorem=start_with_lorem
                )
        return ('<ul>' + _NEWLINE + output + _NEWLINE + '</ul>')

    def generate_sentences_html_li(self, quantity, start_with_lorem=False):
        """Generates a number of sentences surrounded by HTML 'li' tags."""
        output = self.__generate_markup_sentences(
                begin_sentence='\t<li>' + _NEWLINE + '\t\t',
                end_sentence=_NEWLINE + '\t</li>',
                between_sentences=_NEWLINE,
                quantity=quantity,
                start_with_lorem=start_with_lorem
                )
        return ('<ul>' + _NEWLINE + output + _NEWLINE + '</ul>')
