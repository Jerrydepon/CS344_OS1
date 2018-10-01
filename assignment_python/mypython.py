import string
from random import *

# reference: https://www.pythoncentral.io/python-snippets-how-to-generate-random-string/
s1 = "".join(choice(string.ascii_letters.lower()) for x in range(10))
s2 = "".join(choice(string.ascii_letters.lower()) for x in range(10))
s3 = "".join(choice(string.ascii_letters.lower()) for x in range(10))
print(s1)
print(s2)
print(s3)

file_1 = open("file_1","w")
file_1.write(s1 + "\n") 
file_1.close() 

file_2 = open("file_2","w")
file_2.write(s2 + "\n")
file_2.close()

file_3 = open("file_3","w")
file_3.write(s3 + "\n")
file_3.close()

x = randint(1, 42)
y = randint(1, 42)
print(x)
print(y)
print(x * y)
