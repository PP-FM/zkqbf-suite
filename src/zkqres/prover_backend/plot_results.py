import pandas as pd
import matplotlib.pyplot as plt
import sys

# Read the first CSV file.
# Adjust skiprows if necessary; here we assume CSV1 has extra rows at the top.
df1 = pd.read_csv(sys.argv[1], skiprows=3)

# Read the second CSV file that contains just name and t.
df2 = pd.read_csv(sys.argv[2])

# Select only the name and t columns from df1.
df1 = df1[['name', 't']]

# Merge the two dataframes on the "name" column.
merged = pd.merge(df1, df2, on='name', suffixes=('_csv1', '_csv2'))

# (Optional) Sort the merged dataframe by the t value from CSV1 for easier comparison.
merged = merged.sort_values(by='t_csv1', ascending=True)

# Create a numeric x-axis for plotting.
x = range(len(merged))

# Plot the t values from each CSV.
plt.figure(figsize=(12, 6))
plt.plot(x, merged['t_csv1'], marker='o', label='t (CSV1)')
plt.plot(x, merged['t_csv2'], marker='o', label='t (CSV2)')

plt.xlabel('Instances (sorted by CSV1 t)')
plt.ylabel('t values')
plt.title('Comparison of t values for common instances from two CSV files')
plt.legend()
plt.tight_layout()
plt.show()